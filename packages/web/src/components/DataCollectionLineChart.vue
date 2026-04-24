<script setup lang="ts">
import { animate, type JSAnimation } from 'animejs'
import {
  computed,
  nextTick,
  onBeforeUnmount,
  ref,
  watch,
} from 'vue'
import {
  type CollectionTimeSeriesRow,
  mockCollectionSeries,
} from '../mocks/dataCollectionSeries'

const props = withDefaults(
  defineProps<{
    rows?: CollectionTimeSeriesRow[]
  }>(),
  { rows: undefined },
)

const data = computed(
  () => props.rows ?? mockCollectionSeries(),
)

const CHART = {
  vbW: 400,
  vbH: 256,
  innerL: 48,
  innerR: 352,
  topT: 16,
  topB: 128,
  botT: 140,
  botB: 232,
} as const
const { vbW, vbH, innerL, innerR, topT, topB, botT, botB } = CHART
const innerW = innerR - innerL

const wrapRef = ref<HTMLElement | null>(null)
const svgRef = ref<SVGSVGElement | null>(null)
const tooltipRef = ref<HTMLElement | null>(null)
let tipAnim: JSAnimation | null = null

const activeIndex = ref<number | null>(null)
const tooltipX = ref(0)
const clientPointerY = ref(0)
const isHovering = ref(false)
const pointerLeaving = ref(false)

function prefersReducedMotion() {
  if (typeof window === 'undefined') return false
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

function stopTip() {
  tipAnim?.revert()
  tipAnim = null
  const el = tooltipRef.value
  if (el) {
    el.style.removeProperty('opacity')
    el.style.removeProperty('transform')
  }
}

function showTipEnter() {
  const el = tooltipRef.value
  if (!el) return
  stopTip()
  if (prefersReducedMotion()) {
    el.style.opacity = '1'
    el.style.transform = 'translateY(0) scale(1)'
    return
  }
  el.style.opacity = '0'
  el.style.transform = 'translateY(6px) scale(0.98)'
  tipAnim = animate(el, {
    opacity: [0, 1],
    y: [6, 0],
    scale: [0.98, 1],
    duration: 180,
    ease: 'outCubic',
  })
}

function showTipPulse() {
  if (prefersReducedMotion()) return
  const el = tooltipRef.value
  if (!el) return
  stopTip()
  tipAnim = animate(el, {
    opacity: [1, 0.9, 1],
    duration: 120,
    ease: 'inOutCubic',
  })
}

const times = computed(() =>
  data.value.map((r) => {
    const t = Date.parse(r.date + 'T12:00:00')
    return Number.isNaN(t) ? NaN : t
  }),
)

const tMin = computed(() => {
  const v = times.value.filter((x) => !Number.isNaN(x))
  if (v.length === 0) return 0
  return Math.min(...v)
})
const tMax = computed(() => {
  const v = times.value.filter((x) => !Number.isNaN(x))
  if (v.length === 0) return 1
  return Math.max(...v)
})

function xAt(i: number): number {
  const t = times.value[i]
  if (t === undefined || Number.isNaN(t) || tMin.value === tMax.value) {
    return innerL + innerW / 2
  }
  return innerL + ((t - tMin.value) / (tMax.value - tMin.value)) * innerW
}

const xPositions = computed(() => data.value.map((_, i) => xAt(i)))

function domRange(
  get: (r: CollectionTimeSeriesRow) => number | null,
): [number, number] {
  const v = data.value
    .map((r) => get(r))
    .filter((n): n is number => n != null && !Number.isNaN(n))
  if (v.length === 0) return [0, 1]
  let lo = Math.min(...v)
  let hi = Math.max(...v)
  if (lo === hi) {
    lo -= 0.5
    hi += 0.5
  } else {
    const p = (hi - lo) * 0.08
    lo -= p
    hi += p
  }
  return [lo, hi]
}

const aqiR = computed(() => domRange((r) => r.aqi))
const swR = computed(() => domRange((r) => r.shortwave))
const timeR = computed(() => {
  const hrs: number[] = []
  for (const r of data.value) {
    if (r.sunriseMinutes != null && !Number.isNaN(r.sunriseMinutes)) {
      hrs.push(r.sunriseMinutes / 60)
    }
    if (r.sunsetMinutes != null && !Number.isNaN(r.sunsetMinutes)) {
      hrs.push(r.sunsetMinutes / 60)
    }
  }
  if (hrs.length === 0) return [4, 22]
  let lo = Math.min(...hrs)
  let hi = Math.max(...hrs)
  if (lo === hi) {
    lo -= 0.5
    hi += 0.5
  } else {
    const p = (hi - lo) * 0.1
    lo -= p
    hi += p
  }
  return [lo, hi]
})

function yAqi(aqi: number | null | undefined) {
  if (aqi == null || Number.isNaN(aqi)) return null
  const r = aqiR.value
  const lo = r[0]!
  const hi = r[1]!
  return topB - ((aqi - lo) / (hi - lo)) * (topB - topT)
}

function ySw(sw: number | null | undefined) {
  if (sw == null || Number.isNaN(sw)) return null
  const r = swR.value
  const lo = r[0]!
  const hi = r[1]!
  return topB - ((sw - lo) / (hi - lo)) * (topB - topT)
}

function yTime(minutes: number | null) {
  if (minutes == null || Number.isNaN(minutes)) return null
  const h = minutes / 60
  const r = timeR.value
  const lo = r[0]!
  const hi = r[1]!
  return botB - ((h - lo) / (hi - lo)) * (botB - botT)
}

/** 下栏：按「小时」域映射到 SVG y，供刻度横线复用 */
function yHourLine(h: number) {
  const r = timeR.value
  const lo = r[0]!
  const hi = r[1]!
  return botB - ((h - lo) / (hi - lo)) * (botB - botT)
}

function linePath(
  getY: (r: CollectionTimeSeriesRow, i: number) => number | null,
) {
  const out: string[] = []
  let needM = true
  for (let i = 0; i < data.value.length; i++) {
    const y = getY(data.value[i]!, i)
    const x = xPositions.value[i]!
    if (y == null) {
      needM = true
      continue
    }
    if (needM) {
      out.push(`M${x},${y}`)
      needM = false
    } else {
      out.push(`L${x},${y}`)
    }
  }
  return out.join(' ')
}

const pathAqi = computed(() =>
  linePath((r) => yAqi(r.aqi)),
)
const pathSw = computed(() =>
  linePath((r) => ySw(r.shortwave)),
)
const pathRise = computed(() =>
  linePath((r) => yTime(r.sunriseMinutes)),
)
const pathSet = computed(() =>
  linePath((r) => yTime(r.sunsetMinutes)),
)

function tickList(
  lo: number,
  hi: number,
  n: number,
): { v: number; t: string }[] {
  const o: { v: number; t: string }[] = []
  for (let k = 0; k < n; k++) {
    const f = n <= 1 ? 0.5 : k / (n - 1)
    const v = lo + f * (hi - lo)
    o.push({ v, t: v.toFixed(0) })
  }
  return o
}

const ticksAqi = computed(() => {
  const a = aqiR.value
  return tickList(a[0]!, a[1]!, 4)
})
const ticksSw = computed(() => {
  const s = swR.value
  return tickList(s[0]!, s[1]!, 4).map(({ v, t }) => ({
    v,
    t: v.toFixed(1),
  }))
})
const ticksTime = computed(() => {
  const r = timeR.value
  return tickList(r[0]!, r[1]!, 4).map(({ v }) => {
    const m = Math.round(v * 60)
    const h = Math.floor(m / 60) % 24
    const mm = m % 60
    return {
      v,
      t: `${String(h).padStart(2, '0')}:${String(mm).padStart(2, '0')}`,
    }
  })
})

const xTickIndices = computed(() => {
  const n = data.value.length
  if (n === 0) return []
  const want = 5
  if (n <= want) {
    return data.value.map((_, i) => i)
  }
  const o: number[] = []
  for (let k = 0; k < want; k++) {
    o.push(Math.round((k * (n - 1)) / (want - 1)))
  }
  return [...new Set(o)].sort((a, b) => a - b)
})

function shortDate(ymd: string) {
  const p = ymd.split('-')
  if (p.length === 3) return `${p[1]}-${p[2]}`
  return ymd
}

function clientToSvgX(clientX: number, clientY: number) {
  const svg = svgRef.value
  if (!svg) return 0
  const pt = svg.createSVGPoint()
  pt.x = clientX
  pt.y = clientY
  const ctm = svg.getScreenCTM()
  if (!ctm) return innerL
  const p = pt.matrixTransform(ctm.inverse())
  return p.x
}

function nearestIndex(svgX: number) {
  const n = xPositions.value.length
  if (n === 0) return 0
  let best = 0
  let d = Math.abs(xPositions.value[0]! - svgX)
  for (let i = 1; i < n; i++) {
    const d2 = Math.abs(xPositions.value[i]! - svgX)
    if (d2 < d) {
      d = d2
      best = i
    }
  }
  return best
}

function placeTooltip() {
  const w = wrapRef.value
  const tip = tooltipRef.value
  if (!w || !tip) return
  const pad = 10
  const tw = tip.offsetWidth
  const th = tip.offsetHeight
  const r = w.getBoundingClientRect()
  let left = tooltipX.value
  let top = clientPointerY.value - r.top - th - pad
  if (top < pad) {
    top = clientPointerY.value - r.top + pad
  }
  if (left + tw + pad > r.width) {
    left = r.width - tw - pad
  }
  if (left < pad) left = pad
  if (top + th + pad > r.height) {
    top = r.height - th - pad
  }
  tip.style.left = `${left}px`
  tip.style.top = `${top}px`
}

const activeRow = computed(() => {
  const i = activeIndex.value
  if (i == null) return null
  return data.value[i] ?? null
})

let prevForWatch: number | null = null
watch(
  activeIndex,
  (n, o) => {
    if (n == null) {
      prevForWatch = null
      return
    }
    void nextTick(() => {
      placeTooltip()
      if (o == null && n != null) {
        showTipEnter()
      } else if (o != null && o !== n) {
        showTipPulse()
      }
      prevForWatch = n
    })
  },
)

function onPointerMove(e: PointerEvent) {
  if (!wrapRef.value || !svgRef.value) return
  isHovering.value = true
  if (pointerLeaving.value) {
    pointerLeaving.value = false
    stopTip()
    const t = tooltipRef.value
    if (t) {
      t.style.opacity = '1'
      t.style.transform = 'translateY(0) scale(1)'
    }
  }
  const sx = clientToSvgX(e.clientX, e.clientY)
  const i = nearestIndex(sx)
  if (i !== activeIndex.value) {
    activeIndex.value = i
  }
  const w = wrapRef.value.getBoundingClientRect()
  const svgB = svgRef.value.getBoundingClientRect()
  const cx = (xAt(i) / vbW) * svgB.width
  tooltipX.value = cx + svgB.left - w.left
  clientPointerY.value = e.clientY
  void nextTick(() => placeTooltip())
}

function onPointerLeave() {
  isHovering.value = false
  if (activeIndex.value == null) {
    stopTip()
    return
  }
  const el = tooltipRef.value
  if (prefersReducedMotion() || !el) {
    activeIndex.value = null
    stopTip()
    return
  }
  pointerLeaving.value = true
  stopTip()
  tipAnim = animate(el, {
    opacity: [1, 0],
    y: [0, 4],
    duration: 120,
    ease: 'inCubic',
    onComplete: () => {
      pointerLeaving.value = false
      activeIndex.value = null
      if (el) {
        el.style.removeProperty('opacity')
        el.style.removeProperty('transform')
      }
    },
  })
}

onBeforeUnmount(() => {
  stopTip()
})

function fmtHHMM(m: number | null) {
  if (m == null) return '—'
  const h = Math.floor(m / 60) % 24
  const mm = Math.round(m % 60)
  return `${String(h).padStart(2, '0')}:${String(mm).padStart(2, '0')}`
}

function fmtAqi(n: number | null) {
  if (n == null) return '—'
  return String(Math.round(n))
}

function fmtSw(n: number | null) {
  if (n == null) return '—'
  return n.toFixed(1)
}
</script>

<template>
  <div
    ref="wrapRef"
    class="dclc"
  >
    <p class="dclc__title">
      时序环境指标（示例假数据）
    </p>
    <svg
      ref="svgRef"
      class="dclc__svg"
      :viewBox="`0 0 ${vbW} ${vbH}`"
      width="100%"
      height="auto"
      role="img"
      aria-label="环境数据时序折线图，含 AQI、短波辐射、日出与日落；悬停可查看具体数值"
    >
      <rect
        :x="innerL - 2"
        :y="topT - 2"
        :width="innerW + 4"
        :height="topB - topT + 4"
        fill="rgba(61, 90, 66, 0.04)"
        rx="4"
      />
      <text
        x="4"
        :y="topT + 10"
        class="dclc__axis cap"
        fill="currentColor"
      >
        AQI
      </text>
      <g class="dclc__ticks aqi">
        <line
          v-for="(tk, j) in ticksAqi"
          :key="'at' + j"
          :x1="innerL - 4"
          :x2="innerR + 4"
          :y1="yAqi(tk.v)!"
          :y2="yAqi(tk.v)!"
          stroke="rgba(61, 90, 66, 0.12)"
          stroke-width="1"
        />
        <text
          v-for="(tk, j) in ticksAqi"
          :key="'al' + j"
          :x="innerL - 6"
          :y="yAqi(tk.v)! + 3"
          text-anchor="end"
          class="dclc__tick"
        >
          {{ tk.t }}
        </text>
      </g>
      <text
        :x="vbW - 4"
        :y="topT + 10"
        class="dclc__axis cap"
        text-anchor="end"
        fill="currentColor"
      >
        短波 MJ/m²
      </text>
      <g class="dclc__ticks">
        <text
          v-for="(tk, j) in ticksSw"
          :key="'sr' + j"
          :x="innerR + 6"
          :y="ySw(tk.v)! + 3"
          text-anchor="start"
          class="dclc__tick"
        >
          {{ tk.t }}
        </text>
      </g>
      <path
        v-if="pathAqi.length"
        :d="pathAqi"
        fill="none"
        class="dclc__line dclc__line--aqi"
        stroke-width="1.6"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        v-if="pathSw.length"
        :d="pathSw"
        fill="none"
        class="dclc__line dclc__line--sw"
        stroke-width="1.6"
        stroke-linecap="round"
        stroke-linejoin="round"
      />

      <text
        x="4"
        :y="botT - 2"
        class="dclc__axis cap"
        fill="currentColor"
      >
        时刻
      </text>
      <rect
        :x="innerL - 2"
        :y="botT - 2"
        :width="innerW + 4"
        :height="botB - botT + 4"
        fill="rgba(61, 90, 66, 0.04)"
        rx="4"
      />
      <g class="dclc__ticks time">
        <line
          v-for="(tk, j) in ticksTime"
          :key="'bt' + j"
          :x1="innerL - 4"
          :x2="innerR + 4"
          :y1="yHourLine(tk.v)"
          :y2="yHourLine(tk.v)"
          stroke="rgba(61, 90, 66, 0.12)"
          stroke-width="1"
        />
        <text
          v-for="(tk, j) in ticksTime"
          :key="'bl' + j"
          :x="innerL - 6"
          :y="yHourLine(tk.v) + 3"
          text-anchor="end"
          class="dclc__tick"
        >
          {{ tk.t }}
        </text>
      </g>
      <path
        v-if="pathRise.length"
        :d="pathRise"
        fill="none"
        class="dclc__line dclc__line--rise"
        stroke-width="1.6"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        v-if="pathSet.length"
        :d="pathSet"
        fill="none"
        class="dclc__line dclc__line--set"
        stroke-width="1.6"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <g
        v-for="idx in xTickIndices"
        :key="'xl' + idx"
      >
        <text
          :x="xAt(idx)"
          :y="vbH - 4"
          class="dclc__xlabel"
          text-anchor="middle"
        >
          {{ shortDate(data[idx]!.date) }}
        </text>
      </g>
      <g v-if="activeIndex != null && activeRow">
        <line
          :x1="xAt(activeIndex!)"
          :x2="xAt(activeIndex!)"
          :y1="topT"
          :y2="botB"
          class="dclc__cursor"
        />
        <circle
          v-if="yAqi(activeRow.aqi) != null"
          :cx="xAt(activeIndex!)"
          :cy="yAqi(activeRow.aqi)!"
          r="3.2"
          class="dclc__dot dclc__dot--aqi"
        />
        <circle
          v-if="ySw(activeRow.shortwave) != null"
          :cx="xAt(activeIndex!)"
          :cy="ySw(activeRow.shortwave)!"
          r="3.2"
          class="dclc__dot dclc__dot--sw"
        />
        <circle
          v-if="yTime(activeRow.sunriseMinutes) != null"
          :cx="xAt(activeIndex!)"
          :cy="yTime(activeRow.sunriseMinutes)!"
          r="3.2"
          class="dclc__dot dclc__dot--rise"
        />
        <circle
          v-if="yTime(activeRow.sunsetMinutes) != null"
          :cx="xAt(activeIndex!)"
          :cy="yTime(activeRow.sunsetMinutes)!"
          r="3.2"
          class="dclc__dot dclc__dot--set"
        />
      </g>
      <rect
        :x="0"
        :y="0"
        :width="vbW"
        :height="vbH"
        fill="transparent"
        class="dclc__hit"
        @pointermove="onPointerMove"
        @pointerdown="onPointerMove"
        @pointerleave="onPointerLeave"
        @pointercancel="onPointerLeave"
      />
    </svg>
    <ul
      class="dclc__legend"
      aria-hidden="true"
    >
      <li class="aqi">AQI</li>
      <li class="sw">短波</li>
      <li class="rise">日出</li>
      <li class="set">日落</li>
    </ul>
    <div
      v-show="activeIndex != null"
      ref="tooltipRef"
      class="dclc__tip"
      role="status"
      :aria-live="isHovering ? 'polite' : 'off'"
    >
      <template v-if="activeRow">
        <div class="dclc__tip-date">
          {{ activeRow.date }}
        </div>
        <ul class="dclc__tip-list">
          <li>日出 {{ fmtHHMM(activeRow.sunriseMinutes) }}</li>
          <li>日落 {{ fmtHHMM(activeRow.sunsetMinutes) }}</li>
          <li>AQI {{ fmtAqi(activeRow.aqi) }}</li>
          <li>短波（MJ/m²） {{ fmtSw(activeRow.shortwave) }}</li>
        </ul>
      </template>
    </div>
  </div>
</template>

<style scoped>
.dclc {
  position: relative;
  width: 100%;
  min-width: 0;
  color: #3d5a42;
}

.dclc__title {
  margin: 0 0 0.5rem;
  font-size: 0.8125rem;
  font-weight: 500;
  letter-spacing: 0.04em;
  color: #4a6350;
}

.dclc__svg {
  display: block;
  max-width: 100%;
  min-height: 12rem;
}

.dclc__axis {
  font-size: 8px;
  font-weight: 600;
  letter-spacing: 0.06em;
  opacity: 0.85;
}

.dclc__axis.cap {
  text-transform: none;
}

.dclc__tick {
  font-size: 7.5px;
  fill: #5a7360;
}

.dclc__xlabel {
  font-size: 8px;
  fill: #5a7360;
  transform: rotate(-32deg);
  transform-origin: center;
}

.dclc__line--aqi {
  /* 冷色：与短波/日出/日落区分 */
  stroke: #1d4ed8;
}
.dclc__line--sw {
  /* 暖色：短波/辐射 */
  stroke: #c2410c;
}
.dclc__line--rise {
  stroke: #b8860b;
}
.dclc__line--set {
  stroke: #a45c2c;
}
.dclc__cursor {
  stroke: rgba(61, 90, 66, 0.35);
  stroke-width: 1;
  stroke-dasharray: 3 3;
  pointer-events: none;
}
.dclc__dot--aqi {
  fill: #1d4ed8;
}
.dclc__dot--sw {
  fill: #c2410c;
}
.dclc__dot--rise {
  fill: #b8860b;
  stroke: #fff;
  stroke-width: 0.5;
}
.dclc__dot--set {
  fill: #a45c2c;
  stroke: #fff;
  stroke-width: 0.5;
}

.dclc__hit {
  cursor: crosshair;
}

.dclc__legend {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem 1rem;
  margin: 0.4rem 0 0;
  padding: 0;
  list-style: none;
  font-size: 0.7rem;
  color: #5a7360;
}
.dclc__legend li {
  padding-left: 0.9rem;
  position: relative;
}
.dclc__legend li::before {
  content: '';
  position: absolute;
  left: 0;
  top: 0.3em;
  width: 0.5rem;
  height: 2px;
  border-radius: 1px;
}
.dclc__legend .aqi::before {
  background: #1d4ed8;
}
.dclc__legend .sw::before {
  background: #c2410c;
}
.dclc__legend .rise::before {
  background: #b8860b;
}
.dclc__legend .set::before {
  background: #a45c2c;
}

.dclc__tip {
  position: absolute;
  left: 0;
  top: 0;
  z-index: 5;
  min-width: 9rem;
  max-width: 16rem;
  padding: 0.55rem 0.7rem;
  font-size: 0.75rem;
  line-height: 1.5;
  color: #2d3f31;
  background: rgba(255, 255, 255, 0.96);
  border: 1px solid rgba(61, 90, 66, 0.2);
  border-radius: 0.5rem;
  box-shadow: 0 8px 24px rgba(61, 90, 66, 0.15);
  pointer-events: none;
  opacity: 0;
  transform: translateY(4px) scale(0.98);
}

.dclc__tip-date {
  font-weight: 600;
  margin-bottom: 0.2rem;
  color: #3d5a42;
  letter-spacing: 0.04em;
}
.dclc__tip-list {
  margin: 0;
  padding: 0;
  list-style: none;
}
</style>

<script setup lang="ts">
import { animate, type JSAnimation } from 'animejs'
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'

const props = defineProps<{
  labels: string[]
  activeIndex: number
}>()

const DOT_INACTIVE = '#9ca3af'
const DOT_ACTIVE = '#111111'
const DURATION_MS = 600

const progressRef = ref<HTMLElement | null>(null)
const dotEls = ref<(HTMLElement | null)[]>([])

let dotAnims: JSAnimation[] = []
let progressAnim: JSAnimation | null = null

function prefersReducedMotion() {
  if (typeof window === 'undefined') return false
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

function bindDotRef(i: number) {
  return (el: unknown) => {
    dotEls.value[i] = el instanceof HTMLElement ? el : null
  }
}

function stopAnimations() {
  for (const a of dotAnims) a.revert()
  dotAnims = []
  progressAnim?.revert()
  progressAnim = null
}

function clearDotStyles(el: HTMLElement) {
  el.style.removeProperty('backgroundColor')
  el.style.removeProperty('transform')
  el.style.removeProperty('scale')
}

function clearProgressStyles(el: HTMLElement) {
  el.style.removeProperty('transform')
  el.style.removeProperty('scale')
}

function applyReducedMotionState() {
  const n = props.labels.length
  const active = props.activeIndex
  for (let i = 0; i < n; i++) {
    const el = dotEls.value[i]
    if (!el) continue
    clearDotStyles(el)
    el.style.backgroundColor = active === i ? DOT_ACTIVE : DOT_INACTIVE
    el.style.transform = active === i ? 'scale(1.15)' : 'scale(1)'
  }
  const p = progressRef.value
  if (p) {
    clearProgressStyles(p)
    if (n < 2) {
      p.style.transform = 'scaleY(1)'
    }
    else {
      const ratio = active / (n - 1)
      p.style.transform = `scaleY(${ratio})`
    }
  }
}

function progressTargetRatio(): number {
  const n = props.labels.length
  if (n < 2) return 1
  return props.activeIndex / (n - 1)
}

function runAnimations() {
  const n = props.labels.length
  if (n === 0) return

  if (prefersReducedMotion()) {
    stopAnimations()
    applyReducedMotionState()
    return
  }

  stopAnimations()

  for (let i = 0; i < n; i++) {
    const el = dotEls.value[i]
    if (!el) continue
    const isActive = props.activeIndex === i
    const anim = animate(el, {
      backgroundColor: isActive ? DOT_ACTIVE : DOT_INACTIVE,
      scale: isActive ? 1.15 : 1,
      duration: DURATION_MS,
      ease: 'inOutCubic',
    })
    dotAnims.push(anim)
  }

  const p = progressRef.value
  if (p && n >= 2) {
    clearProgressStyles(p)
    progressAnim = animate(p, {
      scaleY: progressTargetRatio(),
      duration: DURATION_MS,
      ease: 'inOutCubic',
    })
  }
  else if (p && n < 2) {
    clearProgressStyles(p)
    progressAnim = animate(p, {
      scaleY: 1,
      duration: DURATION_MS,
      ease: 'inOutCubic',
    })
  }
}

watch(
  () => [props.activeIndex, props.labels.length] as const,
  () => {
    void nextTick(() => runAnimations())
  },
)

onMounted(() => {
  void nextTick(() => runAnimations())
})

onBeforeUnmount(() => {
  stopAnimations()
  for (const el of dotEls.value) {
    if (el) clearDotStyles(el)
  }
  const p = progressRef.value
  if (p) clearProgressStyles(p)
})
</script>

<template>
  <nav
    class="section-timeline"
    aria-label="模块进度"
  >
    <div class="section-timeline__inner">
      <div
        class="section-timeline__rail"
        aria-hidden="true"
      >
        <div class="section-timeline__rail-bg" />
        <div
          ref="progressRef"
          class="section-timeline__rail-fill"
        />
      </div>
      <ol class="section-timeline__list">
        <li
          v-for="(label, i) in labels"
          :key="i"
          class="section-timeline__item"
          :aria-current="activeIndex === i ? 'step' : undefined"
        >
          <span
            :ref="bindDotRef(i)"
            class="section-timeline__dot"
            :title="label"
          />
          <span class="section-timeline__visually-hidden">{{ label }}</span>
        </li>
      </ol>
    </div>
  </nav>
</template>

<style scoped>
.section-timeline {
  position: fixed;
  right: clamp(1rem, 3vw, 2rem);
  top: 50%;
  transform: translateY(-50%);
  z-index: 15;
  pointer-events: none;
}

.section-timeline__inner {
  position: relative;
  width: max-content;
}

.section-timeline__rail {
  position: absolute;
  left: 50%;
  top: 0;
  bottom: 0;
  width: 2px;
  transform: translateX(-50%);
  pointer-events: none;
}

.section-timeline__rail-bg {
  position: absolute;
  inset: 0;
  border-radius: 1px;
  background: rgba(156, 163, 175, 0.45);
}

.section-timeline__rail-fill {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 100%;
  border-radius: 1px;
  background: rgba(107, 133, 112, 0.85);
  transform-origin: top center;
  transform: scaleY(0);
  will-change: transform;
}

.section-timeline__list {
  position: relative;
  z-index: 1;
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 1.25rem;
}

.section-timeline__item {
  margin: 0;
  display: flex;
  justify-content: center;
  align-items: center;
}

.section-timeline__dot {
  display: block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background-color: #9ca3af;
  transform-origin: center center;
  will-change: transform, background-color;
}

.section-timeline__visually-hidden {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

@media (prefers-reduced-motion: reduce) {
  .section-timeline__dot,
  .section-timeline__rail-fill {
    will-change: auto;
  }
}
</style>

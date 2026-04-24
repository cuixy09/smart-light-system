<script setup lang="ts">
import { animate, type JSAnimation } from 'animejs'
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import ScrollModuleContent from './components/ScrollModuleContent.vue'
import SectionTimeline from './components/SectionTimeline.vue'
import TypewriterCursor from './components/TypewriterCursor.vue'

const texts: string[] = ['路灯节能控制方案', '数据收集', '模型展示']
const maxIndex = texts.length - 1

const section = ref(0)
const titleInner = ref<HTMLElement | null>(null)
const scrollHintInner = ref<HTMLElement | null>(null)

/** 与「角标/大标题」排版同步：位移动画结束后再切换，避免宽度与 text-align 瞬时变化干扰测量与观感 */
const titleCompactTypo = ref(false)

const isAnimating = ref(false)
let titleAnim: JSAnimation | null = null
let scrollHintAnim: JSAnimation | null = null

const SCROLL_HINT_BOB_MS = 1400

function prefersReducedMotion() {
  if (typeof window === 'undefined') return false
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

function stopScrollHintAnim() {
  scrollHintAnim?.revert()
  scrollHintAnim = null
  const el = scrollHintInner.value
  if (el) {
    el.style.removeProperty('transform')
    el.style.removeProperty('translate')
  }
}

function startScrollHintAnim() {
  stopScrollHintAnim()
  if (section.value !== 0) return
  if (prefersReducedMotion()) return
  const el = scrollHintInner.value
  if (!el) return
  el.style.removeProperty('transform')
  el.style.removeProperty('translate')
  scrollHintAnim = animate(el, {
    y: [0, 6],
    duration: SCROLL_HINT_BOB_MS,
    ease: 'inOutCubic',
    loop: true,
    alternate: true,
  })
}

const POSITION_DURATION_MS = 720
const STEP_LOCK_MS = 380

function isCornerSection(s: number) {
  return s >= 1
}

function padPx() {
  const rootFs = parseFloat(getComputedStyle(document.documentElement).fontSize || '16')
  return rootFs * 1.5
}

function runTitlePositionTransition(prev: number, next: number) {
  const inner = titleInner.value
  if (!inner) return

  const prevCorner = isCornerSection(prev)
  const nextCorner = isCornerSection(next)
  if (prevCorner === nextCorner) return

  titleAnim?.revert()
  titleAnim = null

  inner.style.removeProperty('translate')
  inner.style.removeProperty('scale')

  const toCorner = next >= 1
  const pad = padPx()

  const startAnim = () => {
    if (toCorner) {
      const rect = inner.getBoundingClientRect()
      const dx = pad - rect.left
      const dy = pad - rect.top
      titleAnim = animate(inner, {
        duration: POSITION_DURATION_MS,
        ease: 'inOutCubic',
        x: [0, dx],
        y: [0, dy],
        scale: [1, 0.88],
        onComplete: () => {
          titleAnim = null
          titleCompactTypo.value = next >= 1
        },
      })
    } else {
      titleAnim = animate(inner, {
        duration: POSITION_DURATION_MS,
        ease: 'inOutCubic',
        x: 0,
        y: 0,
        scale: 1,
        onComplete: () => {
          titleAnim = null
          titleCompactTypo.value = next >= 1
        },
      })
    }
  }

  // 等 flex 布局与上一帧样式稳定后再量 rect，双向（去角标 / 回正中）共用同一套测量逻辑
  requestAnimationFrame(() => {
    requestAnimationFrame(startAnim)
  })
}

function releaseAnimLockAfter(ms: number) {
  window.setTimeout(() => {
    isAnimating.value = false
  }, ms)
}

function onWheel(e: WheelEvent) {
  if (isAnimating.value) {
    e.preventDefault()
    return
  }

  const delta = e.deltaY
  if (delta === 0) return

  const dir = delta > 0 ? 1 : -1
  const next = section.value + dir
  if (next < 0 || next > maxIndex) {
    return
  }

  e.preventDefault()
  isAnimating.value = true

  const prev = section.value
  section.value = next

  const needsPosition = isCornerSection(prev) !== isCornerSection(next)
  if (needsPosition) {
    runTitlePositionTransition(prev, next)
    releaseAnimLockAfter(POSITION_DURATION_MS)
  } else {
    titleCompactTypo.value = next >= 1
    releaseAnimLockAfter(STEP_LOCK_MS)
  }
}

function resetTitleTransform() {
  titleAnim?.revert()
  titleAnim = null
  const inner = titleInner.value
  if (!inner) return
  inner.style.removeProperty('transform')
  inner.style.removeProperty('translate')
  inner.style.removeProperty('scale')
}

watch(
  () => section.value,
  (s) => {
    if (s !== 0) {
      stopScrollHintAnim()
      return
    }
    void nextTick(() => startScrollHintAnim())
  },
)

onMounted(() => {
  nextTick(() => {
    resetTitleTransform()
    if (section.value === 0) startScrollHintAnim()
  })
  window.addEventListener('wheel', onWheel, { passive: false })
})

onBeforeUnmount(() => {
  window.removeEventListener('wheel', onWheel)
  stopScrollHintAnim()
  resetTitleTransform()
})
</script>

<template>
  <main class="hero" :class="{ 'hero--stage': section > 0 }">
    <div class="hero__glow hero__glow--one" aria-hidden="true" />
    <div class="hero__glow hero__glow--two" aria-hidden="true" />

    <div class="title-root">
      <div ref="titleInner" class="title-root__mover">
        <h1 class="title-shell" :class="{ 'title-shell--hero': !titleCompactTypo }">
          <TypewriterCursor
            :texts="texts"
            :index="section"
            :char-interval-ms="100"
            :cursor-blink-interval-ms="530"
          />
        </h1>
      </div>
    </div>

    <div class="hero__content">
      <ScrollModuleContent :section="section" />
      <p v-if="section === 0" class="hero__subtitle">智慧照明 · 节能与可视化</p>
    </div>

    <SectionTimeline :labels="texts" :active-index="section" />

    <div v-if="section === 0" class="hero__scroll-hint-outer">
      <div ref="scrollHintInner" class="hero__scroll-hint-inner" role="note">
        <span class="hero__scroll-hint-text">滚动鼠标滚轮以继续</span>
        <svg
          class="hero__scroll-hint-icon"
          width="20"
          height="20"
          viewBox="0 0 24 24"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
          aria-hidden="true"
        >
          <path
            d="M12 5v14M6 10l6 5 6-5"
            stroke="currentColor"
            stroke-width="1.9"
            stroke-linecap="round"
            stroke-linejoin="round"
          />
        </svg>
      </div>
    </div>
  </main>
</template>

<style scoped>
.hero {
  position: relative;
  min-height: 100vh;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 1.5rem;
  padding-top: clamp(4.5rem, 12vh, 6rem);
  overflow: hidden;
  background: linear-gradient(180deg, #fafdfb 0%, #f3f7f2 45%, #eef3ec 100%);
}

.hero--stage .hero__glow {
  opacity: 0.28;
}

.hero__glow {
  position: absolute;
  border-radius: 50%;
  filter: blur(80px);
  opacity: 0.45;
  pointer-events: none;
  transition: opacity 0.5s ease;
}

.hero__glow--one {
  width: min(72vw, 420px);
  height: min(72vw, 420px);
  top: -10%;
  right: -8%;
  background: radial-gradient(circle, #c8d9c0 0%, transparent 70%);
}

.hero__glow--two {
  width: min(90vw, 520px);
  height: min(90vw, 520px);
  bottom: -18%;
  left: -12%;
  background: radial-gradient(circle, #a3b899 0%, transparent 65%);
}

.title-root {
  position: fixed;
  inset: 0;
  z-index: 20;
  display: flex;
  align-items: center;
  justify-content: center;
  pointer-events: none;
}

.title-root__mover {
  transform-origin: top left;
  will-change: transform;
}

.title-shell {
  margin: 0;
  max-width: min(90vw, 36rem);
  font-size: clamp(1.75rem, 5vw, 2.75rem);
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #3d5a42;
  line-height: 1.35;
  text-align: left;
}

.title-shell--hero {
  text-align: center;
  max-width: min(92vw, 40rem);
  /* 减少打字过程中宽度抖动对「居中」视觉的影响（flex 仍会以当前盒宽居中） */
  min-width: min(16rem, 88vw);
}

.hero__content {
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  width: 100%;
  max-width: 42rem;
  flex: 1;
}

.hero__subtitle {
  margin: 1rem 0 0;
  font-size: 0.9375rem;
  font-weight: 400;
  color: #6b8570;
  letter-spacing: 0.12em;
}

.hero__scroll-hint-outer {
  position: absolute;
  left: 50%;
  bottom: clamp(1.25rem, 4vh, 2.25rem);
  transform: translateX(-50%);
  z-index: 10;
  pointer-events: none;
}

.hero__scroll-hint-inner {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
  font-size: 0.875rem;
  font-weight: 400;
  color: #6b8570;
  letter-spacing: 0.1em;
  will-change: transform;
}

.hero__scroll-hint-text {
  white-space: nowrap;
}

.hero__scroll-hint-icon {
  display: block;
  flex-shrink: 0;
  color: #6b8570;
  opacity: 0.9;
}
</style>

<style>
html,
body,
#app {
  margin: 0;
  min-height: 100%;
}
</style>

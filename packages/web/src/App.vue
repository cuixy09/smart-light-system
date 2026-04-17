<script setup lang="ts">
import { animate, type JSAnimation } from 'animejs'
import { nextTick, onBeforeUnmount, onMounted, ref } from 'vue'
import ScrollModuleContent from './components/ScrollModuleContent.vue'
import TypewriterCursor from './components/TypewriterCursor.vue'

const texts: string[] = ['路灯节能控制方案', '数据收集', '模型展示']
const maxIndex = texts.length - 1

const section = ref(0)
const titleShell = ref<HTMLElement | null>(null)

const isAnimating = ref(false)
let titleAnim: JSAnimation | null = null

const POSITION_DURATION_MS = 580
const STEP_LOCK_MS = 380

function isCornerSection(s: number) {
  return s >= 1
}

function applyTitleLayoutInstant(s: number) {
  const el = titleShell.value
  if (!el) return
  titleAnim?.revert()
  titleAnim = null

  // 仅使用 style.transform，避免与 animejs 写入的 transform 以及 CSS translate/scale 独立属性叠加导致偏移
  el.style.removeProperty('translate')
  el.style.removeProperty('scale')

  if (s === 0) {
    el.style.left = '50%'
    el.style.top = '50%'
    el.style.transform = 'translate(-50%, -50%) scale(1)'
  } else {
    el.style.left = '1.5rem'
    el.style.top = '1.5rem'
    el.style.transform = 'translate(0, 0) scale(0.88)'
  }
}

function runTitlePositionTransition(prev: number, next: number) {
  const el = titleShell.value
  if (!el) return

  const prevCorner = isCornerSection(prev)
  const nextCorner = isCornerSection(next)
  if (prevCorner === nextCorner) return

  titleAnim?.revert()
  titleAnim = null

  const toCenter = next === 0

  el.style.removeProperty('translate')
  el.style.removeProperty('scale')

  titleAnim = animate(el, {
    duration: POSITION_DURATION_MS,
    ease: 'outCubic',
    left: toCenter ? '50%' : '1.5rem',
    top: toCenter ? '50%' : '1.5rem',
    x: toCenter ? '-50%' : '0%',
    y: toCenter ? '-50%' : '0%',
    scale: toCenter ? 1 : 0.88,
    onComplete: () => {
      titleAnim = null
      applyTitleLayoutInstant(next)
    },
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
    releaseAnimLockAfter(STEP_LOCK_MS)
  }
}

onMounted(() => {
  nextTick(() => {
    applyTitleLayoutInstant(section.value)
  })
  window.addEventListener('wheel', onWheel, { passive: false })
})

onBeforeUnmount(() => {
  window.removeEventListener('wheel', onWheel)
  titleAnim?.revert()
  titleAnim = null
})
</script>

<template>
  <main class="hero" :class="{ 'hero--stage': section > 0 }">
    <div class="hero__glow hero__glow--one" aria-hidden="true" />
    <div class="hero__glow hero__glow--two" aria-hidden="true" />

    <h1 ref="titleShell" class="title-shell" :class="{ 'title-shell--hero': section === 0 }">
      <TypewriterCursor
        :texts="texts"
        :index="section"
        :char-interval-ms="100"
        :cursor-blink-interval-ms="530"
      />
    </h1>

    <div class="hero__content">
      <ScrollModuleContent :section="section" />
      <p v-if="section === 0" class="hero__subtitle">智慧照明 · 节能与可视化</p>
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

.title-shell {
  position: fixed;
  z-index: 20;
  margin: 0;
  max-width: min(90vw, 36rem);
  font-size: clamp(1.75rem, 5vw, 2.75rem);
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #3d5a42;
  line-height: 1.35;
  text-align: left;
  pointer-events: none;
  will-change: transform, left, top;
}

.title-shell--hero {
  text-align: center;
  max-width: min(92vw, 40rem);
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
</style>

<style>
html,
body,
#app {
  margin: 0;
  min-height: 100%;
}
</style>

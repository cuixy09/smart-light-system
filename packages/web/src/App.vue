<script setup lang="ts">
import TypewriterCursor from './components/TypewriterCursor.vue'

const heroTitle: string[] = ['路灯节能控制方案']
</script>

<template>
  <main class="hero">
    <div class="hero__glow hero__glow--one" aria-hidden="true" />
    <div class="hero__glow hero__glow--two" aria-hidden="true" />
    <div class="hero__content">
      <h1 class="hero__title">
        <TypewriterCursor
          :texts="heroTitle"
          :index="0"
          :char-interval-ms="100"
          :cursor-blink-interval-ms="530"
        />
      </h1>
      <p class="hero__subtitle">模型展示</p>
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
  overflow: hidden;
  background: linear-gradient(180deg, #fafdfb 0%, #f3f7f2 45%, #eef3ec 100%);
}

.hero__glow {
  position: absolute;
  border-radius: 50%;
  filter: blur(80px);
  opacity: 0.45;
  pointer-events: none;
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

.hero__content {
  position: relative;
  z-index: 1;
  text-align: center;
  max-width: 42rem;
}

.hero__title {
  margin: 0;
  font-size: clamp(1.75rem, 5vw, 2.75rem);
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #3d5a42;
  line-height: 1.35;
}

.hero__subtitle {
  margin: 0.75rem 0 0;
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

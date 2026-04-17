<script setup lang="ts">
import { computed, onBeforeUnmount, ref, watch } from 'vue'

const props = withDefaults(
  defineProps<{
    texts: string[]
    index: number
    cursorBlinkIntervalMs?: number
    charIntervalMs?: number
  }>(),
  {
    cursorBlinkIntervalMs: 530,
    charIntervalMs: 70,
  },
)

const displayedText = ref('')
let typeTimer: ReturnType<typeof setTimeout> | null = null

function clearTypeTimer() {
  if (typeTimer !== null) {
    clearTimeout(typeTimer)
    typeTimer = null
  }
}

const cursorStyle = computed(() => ({
  animationDuration: `${props.cursorBlinkIntervalMs}ms`,
}))

watch(
  () => [props.index, props.texts, props.charIntervalMs] as const,
  (_, __, onCleanup) => {
    onCleanup(() => {
      clearTypeTimer()
    })
    clearTypeTimer()
    displayedText.value = ''

    const target = props.texts[props.index] ?? ''
    let i = 0

    const step = () => {
      if (i >= target.length) {
        typeTimer = null
        return
      }
      displayedText.value = target.slice(0, i + 1)
      i++
      if (i < target.length) {
        typeTimer = setTimeout(step, props.charIntervalMs)
      } else {
        typeTimer = null
      }
    }

    if (target.length > 0) {
      typeTimer = setTimeout(step, 0)
    }
  },
  { immediate: true, flush: 'pre' },
)

onBeforeUnmount(() => {
  clearTypeTimer()
})
</script>

<template>
  <span class="typewriter">
    <span class="typewriter__text">{{ displayedText }}</span
    ><span class="typewriter__cursor" :style="cursorStyle" aria-hidden="true">|</span>
  </span>
</template>

<style scoped>
.typewriter {
  display: inline;
}

.typewriter__cursor {
  display: inline-block;
  animation-name: typewriter-cursor-blink;
  animation-timing-function: ease-in-out;
  animation-iteration-count: infinite;
}

@keyframes typewriter-cursor-blink {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.2;
  }
}
</style>

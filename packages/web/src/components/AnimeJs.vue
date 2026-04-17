<script setup lang="ts">
import { animate, type AnimationParams, type JSAnimation } from 'animejs'
import { onBeforeUnmount, ref, watch } from 'vue'

const props = withDefaults(
  defineProps<{
    /** 传给 anime.js `animate()` 第二参的动画参数 */
    params: AnimationParams
    /** 为 false 时不自动播放，可配合 ref 调用 `run()` */
    autoplay?: boolean
    /** 变更后重新播放（例如按钮自增以重播） */
    playKey?: number | string
    /** 根节点标签，默认 `div` */
    tag?: string
  }>(),
  {
    autoplay: true,
    playKey: 0,
    tag: 'div',
  },
)

const root = ref<HTMLElement | null>(null)
let animation: JSAnimation | null = null

function run() {
  animation?.revert()
  animation = null
  const el = root.value
  if (!el) return
  animation = animate(el, props.params)
}

function revert() {
  animation?.revert()
  animation = null
}

watch(
  () => [props.params, props.playKey, props.autoplay] as const,
  () => {
    if (!props.autoplay) {
      revert()
      return
    }
    run()
  },
  { deep: true, immediate: true, flush: 'post' },
)

onBeforeUnmount(revert)

defineExpose({ run, revert })
</script>

<template>
  <component :is="tag" ref="root" class="anime-js">
    <slot />
  </component>
</template>

<script setup lang="ts">
import BrightnessCalculator from './BrightnessCalculator.vue'

defineProps<{
  section: number
}>()
</script>

<template>
  <div v-if="section === 1 || section === 2" class="scroll-module" aria-live="polite">
    <div v-show="section === 1" class="scroll-module__panel">
      <div class="scroll-module__card">
        <h2 class="scroll-module__heading">数据收集</h2>
        <p class="scroll-module__desc">
          在此接入能耗、光照与路况等多源数据，形成可分析的时序样本，为策略与模型训练提供基础。
        </p>
        <ul class="scroll-module__list">
          <li>能耗与功率曲线</li>
          <li>环境光照与人流热度</li>
          <li>设备状态与告警</li>
        </ul>
      </div>
    </div>
    <div v-show="section === 2" class="scroll-module__panel">
      <div class="scroll-module__card">
        <h2 class="scroll-module__heading">模型展示</h2>
        <p class="scroll-module__desc">
          将采集数据输入节能与调度模型，输出可解释的推荐策略与可视化结果，便于评估与落地。
        </p>
        <BrightnessCalculator />
      </div>
    </div>
  </div>
</template>

<style scoped>
.scroll-module {
  width: 100%;
  max-width: 36rem;
  min-height: 12rem;
}

.scroll-module__panel {
  animation: scroll-module-fade 0.45s ease-out both;
}

.scroll-module__card {
  padding: 1.5rem 1.75rem;
  border-radius: 1rem;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(61, 90, 66, 0.12);
  box-shadow: 0 12px 40px rgba(61, 90, 66, 0.08);
}

.scroll-module__heading {
  margin: 0 0 0.75rem;
  font-size: 1.125rem;
  font-weight: 600;
  color: #3d5a42;
  letter-spacing: 0.06em;
}

.scroll-module__desc {
  margin: 0 0 1rem;
  font-size: 0.9375rem;
  line-height: 1.65;
  color: #4a6350;
}

.scroll-module__list {
  margin: 0;
  padding-left: 1.25rem;
  font-size: 0.875rem;
  line-height: 1.7;
  color: #5a7360;
}

@keyframes scroll-module-fade {
  from {
    opacity: 0;
    transform: translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}
</style>

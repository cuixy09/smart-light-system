<script setup lang="ts">
import axios from 'axios'
import { ref, computed } from 'vue'

const active = ref(true)
const date = ref(new Date().toISOString().split('T')[0])
const hour = ref(new Date().getHours())
const minute = ref(new Date().getMinutes())
const aqi = ref(85)
const sunrise = ref('06:00')
const sunset = ref('18:00')

const loading = ref(false)
const result = ref<{
  brightness: number
  isNight: boolean
  debug?: {
    curMin: number
    sunriseMin: number
    sunsetMin: number
    elapsed: number
    nightTime: number
    lambda: number
    base: number
    aqiFac: number
  }
} | null>(null)
const error = ref<string | null>(null)
const saved = ref(false)
const saveError = ref<string | null>(null)

const brightnessLevel = computed(() => {
  if (!result.value) return 0
  return Math.round((result.value.brightness / 255) * 100)
})

const levelLabel = computed(() => {
  if (!result.value) return '-'
  if (result.value.brightness === 0) return '关闭'
  if (result.value.brightness < 64) return '低'
  if (result.value.brightness < 128) return '中'
  if (result.value.brightness < 192) return '高'
  return '全亮'
})

async function calculate() {
  loading.value = true
  error.value = null
  result.value = null
  saved.value = false
  saveError.value = null

  try {
    const response = await axios.post('/api/cal_brightness', {
      active: active.value,
      date: date.value,
      hour: hour.value,
      minute: minute.value,
      aqi: aqi.value,
      sunrise: sunrise.value,
      sunset: sunset.value,
    })
    result.value = response.data

    // 保存手动设置
    try {
      await axios.post('/api/set_manual', {
        active: active.value,
        date: date.value,
        hour: hour.value,
        minute: minute.value,
        aqi: aqi.value,
        sunrise: sunrise.value,
        sunset: sunset.value,
      })
      saved.value = true
    } catch {
      saveError.value = '数据已计算但保存失败'
    }
  } catch (e) {
    error.value = e instanceof Error ? e.message : '请求失败'
  } finally {
    loading.value = false
  }
}

function padZero(n: number) {
  return n.toString().padStart(2, '0')
}
</script>

<template>
  <div class="calc">
    <div class="calc__controls">
      <label class="calc__row">
        <span class="calc__label">启用</span>
        <input type="checkbox" v-model="active" class="calc__checkbox" />
      </label>

      <label class="calc__row">
        <span class="calc__label">日期</span>
        <input type="date" v-model="date" class="calc__input" />
      </label>

      <div class="calc__row calc__row--group">
        <span class="calc__label">时间</span>
        <div class="calc__time-group">
          <select v-model="hour" class="calc__select">
            <option v-for="h in 24" :key="h-1" :value="h-1">{{ padZero(h-1) }}</option>
          </select>
          <span class="calc__sep">:</span>
          <select v-model="minute" class="calc__select">
            <option v-for="m in 60" :key="m-1" :value="m-1">{{ padZero(m-1) }}</option>
          </select>
        </div>
      </div>

      <label class="calc__row">
        <span class="calc__label">AQI</span>
        <input type="number" v-model.number="aqi" min="0" max="500" class="calc__input calc__input--number" />
      </label>

      <div class="calc__row calc__row--group">
        <span class="calc__label">日出/日落</span>
        <div class="calc__time-group">
          <input type="time" v-model="sunrise" class="calc__input calc__input--time" />
          <span class="calc__sep">/</span>
          <input type="time" v-model="sunset" class="calc__input calc__input--time" />
        </div>
      </div>

      <button @click="calculate" :disabled="loading" class="calc__btn">
        {{ loading ? '计算中...' : '计算亮度' }}
      </button>
    </div>

    <div v-if="error" class="calc__error">{{ error }}</div>

    <div v-if="saved" class="calc__saved">设置已保存</div>
    <div v-if="saveError" class="calc__save-error">{{ saveError }}</div>

    <div v-if="result" class="calc__result">
      <div class="calc__brightness-bar">
        <div class="calc__brightness-fill" :style="{ width: brightnessLevel + '%' }"></div>
      </div>
      <div class="calc__brightness-info">
        <span class="calc__brightness-value">{{ result.brightness }}</span>
        <span class="calc__brightness-level">{{ levelLabel }}</span>
      </div>
      <div class="calc__debug" v-if="result.debug">
        <div>夜间: {{ result.isNight ? '是' : '否' }}</div>
        <div>当前: {{ result.debug.curMin }}分钟 | 日出: {{ result.debug.sunriseMin }} | 日落: {{ result.debug.sunsetMin }}</div>
        <div>elapsed: {{ result.debug.elapsed }} | nightTime: {{ result.debug.nightTime }}</div>
        <div>lambda: {{ result.debug.lambda.toFixed(3) }} | base: {{ result.debug.base.toFixed(1) }}</div>
        <div>aqiFac: {{ result.debug.aqiFac.toFixed(3) }}</div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.calc {
  margin-top: 1rem;
  padding: 1.25rem;
  border-radius: 0.75rem;
  background: rgba(255, 255, 255, 0.5);
  border: 1px solid rgba(61, 90, 66, 0.1);
}

.calc__controls {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.calc__row {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.calc__row--group {
  flex-wrap: wrap;
}

.calc__label {
  min-width: 4rem;
  font-size: 0.875rem;
  color: #5a7360;
}

.calc__input {
  padding: 0.4rem 0.6rem;
  border: 1px solid rgba(61, 90, 66, 0.2);
  border-radius: 0.4rem;
  font-size: 0.875rem;
  background: #fff;
  color: #3d5a42;
}

.calc__input:focus {
  outline: none;
  border-color: #7a9a7d;
}

.calc__input--number {
  width: 5rem;
}

.calc__input--time {
  width: 7rem;
}

.calc__checkbox {
  width: 1.1rem;
  height: 1.1rem;
  accent-color: #7a9a7d;
}

.calc__time-group {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.calc__select {
  padding: 0.4rem 0.5rem;
  border: 1px solid rgba(61, 90, 66, 0.2);
  border-radius: 0.4rem;
  font-size: 0.875rem;
  background: #fff;
  color: #3d5a42;
}

.calc__sep {
  color: #7a9a7d;
  font-weight: 500;
}

.calc__btn {
  margin-top: 0.5rem;
  padding: 0.6rem 1rem;
  border: none;
  border-radius: 0.5rem;
  background: #5a7360;
  color: #fff;
  font-size: 0.9375rem;
  font-weight: 500;
  cursor: pointer;
  transition: background 0.2s;
}

.calc__btn:hover:not(:disabled) {
  background: #4a6350;
}

.calc__btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.calc__error {
  margin-top: 0.75rem;
  padding: 0.5rem 0.75rem;
  border-radius: 0.4rem;
  background: #fde8e8;
  color: #c53030;
  font-size: 0.875rem;
}

.calc__saved {
  margin-top: 0.5rem;
  padding: 0.4rem 0.75rem;
  border-radius: 0.4rem;
  background: #e6f4ea;
  color: #2e7d32;
  font-size: 0.875rem;
}

.calc__save-error {
  margin-top: 0.5rem;
  padding: 0.4rem 0.75rem;
  border-radius: 0.4rem;
  background: #fff3e0;
  color: #e65100;
  font-size: 0.875rem;
}

.calc__result {
  margin-top: 1rem;
  padding-top: 1rem;
  border-top: 1px solid rgba(61, 90, 66, 0.1);
}

.calc__brightness-bar {
  height: 0.5rem;
  border-radius: 0.25rem;
  background: rgba(61, 90, 66, 0.1);
  overflow: hidden;
}

.calc__brightness-fill {
  height: 100%;
  background: linear-gradient(90deg, #7a9a7d, #5a7360);
  transition: width 0.3s ease;
}

.calc__brightness-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 0.5rem;
}

.calc__brightness-value {
  font-size: 1.5rem;
  font-weight: 700;
  color: #3d5a42;
}

.calc__brightness-level {
  font-size: 0.875rem;
  padding: 0.2rem 0.6rem;
  border-radius: 1rem;
  background: rgba(61, 90, 66, 0.1);
  color: #5a7360;
}

.calc__debug {
  margin-top: 0.75rem;
  padding: 0.75rem;
  border-radius: 0.4rem;
  background: rgba(61, 90, 66, 0.05);
  font-size: 0.75rem;
  color: #5a7360;
  line-height: 1.6;
}
</style>

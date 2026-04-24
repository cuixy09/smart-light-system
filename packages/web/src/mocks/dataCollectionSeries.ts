/** 数据收集时序行：与 BrightnessCalculator 的日期风格一致；缺失用 `null`（入站时可将 NaN 规约此类型） */
export type CollectionTimeSeriesRow = {
  date: string
  sunriseMinutes: number | null
  sunsetMinutes: number | null
  aqi: number | null
  /** 短波辐射总量，单位示例：MJ/m² */
  shortwave: number | null
}

export function nullIfNaN(n: number | null | undefined): number | null {
  if (n == null) return null
  return Number.isNaN(n) ? null : n
}

const day = 86_400_000

function addDays(ymd: string, delta: number): string {
  const d = new Date(ymd + 'T12:00:00')
  d.setTime(d.getTime() + delta * day)
  const y = d.getFullYear()
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const dd = String(d.getDate()).padStart(2, '0')
  return `${y}-${m}-${dd}`
}

/** 约 16 天，含跳日与若干 `null` 以验证断线 / 提示框 */
export function mockCollectionSeries(): CollectionTimeSeriesRow[] {
  const base = '2026-01-10'
  const days = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 25,
  ]
  return days.map((ix, k) => {
    const d = addDays(base, ix)
    const t = (k * 0.7) % 6
    return {
      date: d,
      sunriseMinutes: k === 4 || k === 12 ? null : nullIfNaN(360 + k * 2 + t * 3),
      sunsetMinutes: k === 7 ? null : nullIfNaN(1050 - k * 1.2 + t * 2),
      aqi: k === 3 || k === 14 || k === 20 ? null : nullIfNaN(55 + (k % 5) * 8 + t * 4),
      shortwave: k === 5 || k === 11 || k === 19 ? null : nullIfNaN(12.2 + t * 0.8 + (k % 3) * 0.4),
    }
  })
}

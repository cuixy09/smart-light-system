import express from "express";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";

// exe 运行时 process.execPath 指向 exe 自身
const rootDir = dirname(process.execPath);
const appRoot = rootDir;
const staticRoot = join(rootDir, "web");
const indexHtml = join(staticRoot, "index.html");

if (!existsSync(indexHtml)) {
  console.error(
    `[web-server] 未找到 ${indexHtml}（应用根: ${appRoot}）。请将前端构建产物（dist 内的全部文件）复制到与可执行文件/入口同级的 web/ 目录后再启动。`,
  );
  process.exit(1);
}

const app = express();
const startPort = Number(process.env.PORT) || 3000;
const maxPortAttempts = 100;
const portFromEnv =
  process.env.PORT !== undefined && process.env.PORT !== "";

app.use(express.json());

/** 与前端 `type="time"` / cal_brightness 一致：`HH:mm` 或 `HH:mm:ss`（秒参与舍入到分钟） */
function parseTimeToMin(time: string): number {
  const parts = time.split(":").map(Number);
  const h = parts[0] ?? 0;
  const m = parts[1] ?? 0;
  const sec = parts[2];
  if (sec !== undefined && !Number.isNaN(sec)) {
    return Math.round(h * 60 + m + sec / 60);
  }
  return h * 60 + m;
}

type CollectionTimeSeriesRow = {
  date: string;
  sunriseMinutes: number | null;
  sunsetMinutes: number | null;
  aqi: number | null;
  shortwave: number | null;
};

const COL_DATE = "日期";
const COL_SUNRISE = "日出时间";
const COL_SUNSET = "日落时间";
const COL_AQI = "空气质量AQI";
const COL_SHORTWAVE = "短波辐射总量";
const COLLECTION_CSV_FILE = join(rootDir, "1.csv");

function parseTimeCell(s: string): number | null {
  const t = s.trim();
  if (!t) return null;
  const parts = t.split(":").map(Number);
  if (parts.length < 2) return null;
  const h = parts[0]!;
  const m = parts[1]!;
  const sec = parts[2];
  if (Number.isNaN(h) || Number.isNaN(m)) return null;
  if (sec !== undefined && !Number.isNaN(sec)) {
    return Math.round(h * 60 + m + sec / 60);
  }
  return h * 60 + m;
}

function normalizeDateCell(s: string): string | null {
  const t = s.trim();
  if (!t) return null;
  const slash = t.split("/").map((x) => x.trim());
  if (slash.length === 3) {
    const y = Number(slash[0]);
    const mo = Number(slash[1]);
    const d = Number(slash[2]);
    if ([y, mo, d].some((n) => Number.isNaN(n))) return null;
    return `${y}-${String(mo).padStart(2, "0")}-${String(d).padStart(2, "0")}`;
  }
  const dash = t.split("-").map((x) => x.trim());
  if (dash.length === 3) {
    const y = Number(dash[0]);
    const mo = Number(dash[1]);
    const d = Number(dash[2]);
    if ([y, mo, d].some((n) => Number.isNaN(n))) return null;
    return `${y}-${String(mo).padStart(2, "0")}-${String(d).padStart(2, "0")}`;
  }
  return null;
}

function parseNumberCell(s: string): number | null {
  const t = s.trim();
  if (!t) return null;
  const n = Number(t);
  return Number.isFinite(n) ? n : null;
}

function splitCsvLine(line: string): string[] {
  return line.split(",").map((c) => c.trim());
}

function readCollectionCsv(): CollectionTimeSeriesRow[] | null {
  if (!existsSync(COLLECTION_CSV_FILE)) return null;
  let text: string;
  try {
    text = readFileSync(COLLECTION_CSV_FILE, "utf-8");
  } catch {
    return null;
  }
  if (text.charCodeAt(0) === 0xfeff) {
    text = text.slice(1);
  }
  const lines = text.split(/\r?\n/).filter((l) => l.trim() !== "");
  if (lines.length < 2) return null;

  const header = splitCsvLine(lines[0]!);
  const iDate = header.indexOf(COL_DATE);
  const iSunrise = header.indexOf(COL_SUNRISE);
  const iSunset = header.indexOf(COL_SUNSET);
  const iAqi = header.indexOf(COL_AQI);
  const iSw = header.indexOf(COL_SHORTWAVE);
  if (iDate < 0 || iSunrise < 0 || iSunset < 0 || iAqi < 0 || iSw < 0) {
    return null;
  }

  const rows: CollectionTimeSeriesRow[] = [];
  for (let li = 1; li < lines.length; li++) {
    const cells = splitCsvLine(lines[li]!);
    const date = normalizeDateCell(cells[iDate] ?? "");
    if (!date) continue;
    rows.push({
      date,
      sunriseMinutes: parseTimeCell(cells[iSunrise] ?? ""),
      sunsetMinutes: parseTimeCell(cells[iSunset] ?? ""),
      aqi: parseNumberCell(cells[iAqi] ?? ""),
      shortwave: parseNumberCell(cells[iSw] ?? ""),
    });
  }

  return rows.length > 0 ? rows : null;
}

app.post("/api/cal_brightness", (req, res) => {
  const { active, hour, minute, aqi, sunrise, sunset } = req.body;

  if (!active) {
    res.json({ brightness: 0, isNight: false });
    return;
  }

  const sunriseMin = parseTimeToMin(sunrise);
  const sunsetMin = parseTimeToMin(sunset);
  const curMin = hour * 60 + minute;

  const isNight = curMin >= sunsetMin || curMin < sunriseMin;

  if (!isNight) {
    res.json({ brightness: 0, isNight: false });
    return;
  }

  let elapsed: number;
  if (curMin >= sunsetMin) {
    elapsed = curMin - sunsetMin;
  } else {
    elapsed = (24 * 60 - sunsetMin) + curMin;
  }

  const nightTime = (24 * 60 - sunsetMin) + sunriseMin;
  const lambda = elapsed / nightTime;
  const base = 255 * (1 - 2 * Math.abs(lambda - 0.5));

  let aqiFac = 1 + (aqi - 50) / 200;
  if (aqiFac < 0.5) aqiFac = 0.5;
  if (aqiFac > 2.0) aqiFac = 2.0;

  let brightness = Math.round(base * aqiFac);
  if (brightness > 255) brightness = 255;
  if (brightness < 0) brightness = 0;

  res.json({
    brightness,
    isNight: true,
    debug: { curMin, sunriseMin, sunsetMin, elapsed, nightTime, lambda, base, aqiFac },
  });
});

const MANUAL_DATA_FILE = join(rootDir, "manual_data.json");

function readManualData(): Record<string, unknown> | null {
  if (!existsSync(MANUAL_DATA_FILE)) return null;
  try {
    return JSON.parse(readFileSync(MANUAL_DATA_FILE, "utf-8"));
  } catch {
    return null;
  }
}

function writeManualData(data: Record<string, unknown>): void {
  writeFileSync(MANUAL_DATA_FILE, JSON.stringify(data, null, 2));
}

app.post("/api/set_manual", (req, res) => {
  const { active, date, hour, minute, aqi, sunrise, sunset } = req.body;

  if (typeof active !== "boolean") {
    res.status(400).json({ error: "active must be a boolean" });
    return;
  }

  if (!date || typeof date !== "string") {
    res.status(400).json({ error: "date is required" });
    return;
  }

  const data = { active, date, hour, minute, aqi, sunrise, sunset };
  writeManualData(data);

  res.json({ success: true, data });
});

app.get("/api/get_manual", (req, res) => {
  const data = readManualData();
  if (!data || !(data as Record<string, unknown>).active) {
    res.json({ active: false });
    return;
  }
  res.json(data);
});

const COLLECTION_ERR = "无法获取数据";

app.get("/api/collection_timeseries", (_req, res) => {
  const rows = readCollectionCsv();
  if (rows == null) {
    res.status(200).json({ ok: false, error: COLLECTION_ERR });
    return;
  }
  res.status(200).json({ ok: true, rows });
});

app.use(express.static(staticRoot));

app.use((req, res, next) => {
  if (req.method !== "GET" && req.method !== "HEAD") {
    next();
    return;
  }
  if (/\.\w+$/.test(req.path)) {
    res.status(404).end();
    return;
  }
  res.sendFile(indexHtml);
});

function attemptListen(currentPort: number, attemptIndex: number): void {
  if (attemptIndex >= maxPortAttempts) {
    const lastTried = startPort + maxPortAttempts - 1;
    console.error(
      `[web-server] 在 ${maxPortAttempts} 次尝试内未能绑定可用端口（约 ${startPort}–${lastTried}）。`,
    );
    process.exit(1);
    return;
  }

  const server = app.listen(currentPort, () => {
    console.log(
      `[web-server] 监听成功，http://localhost:${currentPort} 应用根: ${appRoot} 静态根: ${staticRoot}`,
    );
    if (currentPort !== startPort) {
      console.log(
        `[web-server] 起始端口 ${startPort} 曾被占用，当前使用端口 ${currentPort}。`,
      );
    }
  });

  server.on("error", (err: NodeJS.ErrnoException) => {
    if (err.code === "EADDRINUSE") {
      console.warn(
        `[web-server] 端口 ${currentPort} 已被占用，尝试 ${currentPort + 1}`,
      );
      server.close(() => {
        attemptListen(currentPort + 1, attemptIndex + 1);
      });
      return;
    }
    console.error("[web-server] 监听失败:", err);
    process.exit(1);
  });
}

console.log(
  `[web-server] 开始尝试绑定端口，起始: ${startPort}${portFromEnv ? "（环境变量 PORT）" : "（未设置 PORT，默认 3000）"}`,
);
attemptListen(startPort, 0);

import express from "express";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";

// @ts-ignore - Bun runtime
const appRoot = import.meta.dir;
// const isBunFs = appRoot.includes("$bunfs") || appRoot.includes("~BUN");
// const rootDir = isBunFs ? dirname(process.execPath) : appRoot;
const rootDir = appRoot;
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

app.post("/api/cal_brightness", (req, res) => {
  const { active, hour, minute, aqi, sunrise, sunset } = req.body;

  if (!active) {
    res.json({ brightness: 0, isNight: false });
    return;
  }

  const parseTimeToMin = (time: string): number => {
    const parts = time.split(":").map(Number);
    const h = parts[0] ?? 0;
    const m = parts[1] ?? 0;
    return h * 60 + m;
  };

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

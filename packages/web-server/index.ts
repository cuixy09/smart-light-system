import express from "express";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";

function appRootDir(): string {
  const metaDir = import.meta.dir;
  const metaPath =
    typeof import.meta === "object" &&
    import.meta !== null &&
    "path" in import.meta &&
    typeof (import.meta as { path: unknown }).path === "string"
      ? (import.meta as { path: string }).path
      : "";
  const inBunVirtualRoot =
    metaDir.includes("~BUN") ||
    metaDir.includes("$bunfs") ||
    metaPath.includes("$bunfs") ||
    metaPath.includes("~BUN");
  if (inBunVirtualRoot) {
    return dirname(process.execPath);
  }
  return metaDir;
}

const appRoot = appRootDir();
const staticRoot = join(appRoot, "web");
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

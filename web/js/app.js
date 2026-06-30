import { TB1EBluetooth, isWebBluetoothSupported } from "./bluetooth.js";

const APP_VERSION = "1.0";
const PANEL_LAYOUT = {
  machineWidthRatio: 0.58,
  assetWidth: 590,
  assetHeight: 2425,
  circleX: 0.813,
  circleUpY: 0.656,
  circleStopY: 0.695,
  circleDownY: 0.733,
  btnUpY: 0.255,
  btnStopY: 0.435,
  btnDownY: 0.615,
  junctionFraction: 0.35,
  buttonHeight: 50,
};

const state = {
  connection: "disconnected",
  lastSent: null,
  lastReceived: null,
  lastError: null,
};

const bt = new TB1EBluetooth({
  onStatus: (status) => {
    state.connection = status;
    state.lastError = null;
    updateUi();
  },
  onMessage: (message) => {
    state.lastReceived = message;
    updateMenuMeta();
  },
  onError: (message) => {
    state.lastError = message;
    state.connection = "error";
    updateUi();
  },
});

const els = {
  version: document.getElementById("app-version"),
  unsupported: document.getElementById("unsupported-banner"),
  connectBtn: document.getElementById("connect-btn"),
  statusDot: document.getElementById("status-dot"),
  statusText: document.getElementById("status-text"),
  errorText: document.getElementById("error-text"),
  menuBtn: document.getElementById("menu-btn"),
  menu: document.getElementById("menu"),
  menuBackdrop: document.getElementById("menu-backdrop"),
  menuMeta: document.getElementById("menu-meta"),
  leaderLines: document.getElementById("leader-lines"),
  panel: document.getElementById("panel"),
  controlButtons: Array.from(document.querySelectorAll(".control-btn")),
};

function statusLabel(connection) {
  switch (connection) {
    case "connected":
      return "Status: Connected";
    case "connecting":
      return "Status: Connecting…";
    case "error":
      return "Status: Error";
    default:
      return "Status: Disconnected";
  }
}

function connectLabel(connection) {
  switch (connection) {
    case "connecting":
      return "ĐANG KẾT NỐI…";
    case "connected":
      return "NGẮT KẾT NỐI";
    default:
      return "KẾT NỐI MÁY THỬ DÂY AN TOÀN";
  }
}

function updateUi() {
  els.version.textContent = `v${APP_VERSION}`;
  els.statusText.textContent = statusLabel(state.connection);
  els.statusDot.className = `status-dot ${state.connection}`;
  els.connectBtn.textContent = connectLabel(state.connection);
  els.connectBtn.disabled = state.connection === "connecting";
  els.errorText.textContent = state.lastError || "";

  const enabled = state.connection === "connected";
  els.controlButtons.forEach((btn) => {
    btn.disabled = !enabled;
  });
}

function updateMenuMeta() {
  const parts = [];
  if (state.lastSent) parts.push(`Last sent: ${state.lastSent}`);
  if (state.lastReceived) parts.push(`Last received: ${state.lastReceived}`);
  els.menuMeta.textContent = parts.join(" · ");
}

function openMenu() {
  els.menu.classList.add("open");
  els.menuBackdrop.classList.add("open");
}

function closeMenu() {
  els.menu.classList.remove("open");
  els.menuBackdrop.classList.remove("open");
}

async function toggleConnect() {
  if (state.connection === "connected") {
    await bt.disconnect(true);
    return;
  }

  try {
    await bt.connect();
  } catch (error) {
    if (error?.name === "NotFoundError") {
      state.lastError = "Không chọn TB-1E.";
    } else if (error?.name === "SecurityError") {
      state.lastError = "Cần mở trang qua HTTPS (không phải http:// IP).";
    } else {
      state.lastError = error?.message || "Không kết nối được TB-1E.";
    }
    state.connection = "error";
    updateUi();
  }
}

async function sendCommand(command) {
  try {
    await bt.send(command);
    state.lastSent = command;
    updateMenuMeta();
  } catch (error) {
    state.lastError = `Send failed: ${error?.message || "unknown"}`;
    state.connection = "error";
    updateUi();
  }
}

function bindMomentaryButton(button, pressCommand) {
  let pressed = false;

  const onPress = async () => {
    if (button.disabled || pressed) return;
    pressed = true;
    await sendCommand(pressCommand);
  };

  const onRelease = async () => {
    if (!pressed) return;
    pressed = false;
    await sendCommand("RELEASE");
  };

  button.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    onPress();
  });

  button.addEventListener("pointerup", onRelease);
  button.addEventListener("pointercancel", onRelease);
  button.addEventListener("lostpointercapture", onRelease);
}

function drawLeaderLines() {
  const panelRect = els.panel.getBoundingClientRect();
  const machineWidth = panelRect.width * PANEL_LAYOUT.machineWidthRatio;
  const controlButtons = {
    up: document.querySelector('.control-btn[data-action="up"]'),
    stop: document.querySelector('.control-btn[data-action="stop"]'),
    down: document.querySelector('.control-btn[data-action="down"]'),
  };

  const svgNs = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(svgNs, "svg");
  svg.setAttribute("width", "100%");
  svg.setAttribute("height", "100%");
  svg.setAttribute("viewBox", `0 0 ${panelRect.width} ${panelRect.height}`);

  function mapImagePoint(xFraction, yFraction) {
    const scale = Math.min(
      machineWidth / PANEL_LAYOUT.assetWidth,
      panelRect.height / PANEL_LAYOUT.assetHeight,
    );
    const displayedWidth = PANEL_LAYOUT.assetWidth * scale;
    const displayedHeight = PANEL_LAYOUT.assetHeight * scale;
    const offsetX = (machineWidth - displayedWidth) / 2;
    const offsetY = (panelRect.height - displayedHeight) / 2;
    return {
      x: offsetX + xFraction * displayedWidth,
      y: offsetY + yFraction * displayedHeight,
    };
  }

  function buttonCenterY(fraction) {
    return panelRect.height * fraction + PANEL_LAYOUT.buttonHeight / 2;
  }

  function drawLine(from, buttonY, buttonLeft) {
    const junctionX =
      from.x + (buttonLeft - from.x) * PANEL_LAYOUT.junctionFraction;
    const line = document.createElementNS(svgNs, "polyline");
    line.setAttribute(
      "points",
      `${from.x},${from.y} ${junctionX},${buttonY} ${buttonLeft},${buttonY}`,
    );
    line.setAttribute("fill", "none");
    line.setAttribute("stroke", "#000");
    line.setAttribute("stroke-width", "0.75");
    svg.appendChild(line);
  }

  const circles = {
    up: mapImagePoint(PANEL_LAYOUT.circleX, PANEL_LAYOUT.circleUpY),
    stop: mapImagePoint(PANEL_LAYOUT.circleX, PANEL_LAYOUT.circleStopY),
    down: mapImagePoint(PANEL_LAYOUT.circleX, PANEL_LAYOUT.circleDownY),
  };

  const buttonYs = {
    up: buttonCenterY(PANEL_LAYOUT.btnUpY),
    stop: buttonCenterY(PANEL_LAYOUT.btnStopY),
    down: buttonCenterY(PANEL_LAYOUT.btnDownY),
  };

  ["up", "stop", "down"].forEach((key) => {
    const btnRect = controlButtons[key].getBoundingClientRect();
    const buttonLeft = btnRect.left - panelRect.left;
    drawLine(circles[key], buttonYs[key], buttonLeft);
  });

  els.leaderLines.replaceChildren(svg);
}

function showUnsupportedBanner() {
  const secure = window.isSecureContext;
  const supported = isWebBluetoothSupported();

  if (!secure) {
    els.unsupported.classList.add("show");
    els.unsupported.textContent =
      "Web Bluetooth cần HTTPS. Mở trang bằng https:// (xem web/README.md). Safari iPhone không hỗ trợ — dùng Chrome Android hoặc app iOS.";
    return;
  }

  if (!supported) {
    els.unsupported.classList.add("show");
    els.unsupported.textContent =
      "Safari iPhone không hỗ trợ Web Bluetooth. Dùng Chrome trên Android, hoặc app iOS/Android trong project.";
  }
}

function init() {
  updateUi();
  showUnsupportedBanner();
  drawLeaderLines();
  window.addEventListener("resize", drawLeaderLines);

  els.connectBtn.addEventListener("click", toggleConnect);
  els.menuBtn.addEventListener("click", openMenu);
  els.menuBackdrop.addEventListener("click", closeMenu);

  bindMomentaryButton(
    document.querySelector('.control-btn[data-action="up"]'),
    "UP",
  );
  bindMomentaryButton(
    document.querySelector('.control-btn[data-action="stop"]'),
    "STOP",
  );
  bindMomentaryButton(
    document.querySelector('.control-btn[data-action="down"]'),
    "DOWN",
  );

  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState !== "visible" && state.connection === "connected") {
      bt.send("RELEASE");
    }
  });

  window.addEventListener("pagehide", () => {
    if (state.connection === "connected") {
      bt.disconnect(true);
    }
  });
}

init();

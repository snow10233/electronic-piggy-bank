# Electronic Piggy Bank

電子存錢筒專案，包含 ESP32/Arduino 韌體、Web 後端 API、前端資料介面與人臉辨識模型資源。

## 專案結構

```text
.
├── firmware/
│   ├── electronic_piggy_bank/      # 主 ESP32 韌體
│   └── examples/                   # 硬體功能測試草稿
├── web/
│   ├── assets/                     # 人臉偵測模型等靜態資源
│   ├── backend/                    # Flask API、資料庫、序列埠、人臉辨識
│   └── frontend/                   # 前端頁面
├── .env.example                    # 本機環境變數範本
└── pyproject.toml                  # uv / Python 依賴設定
```

## 後端啟動

```bash
uv sync
cp .env.example .env
uv run python -m web.backend.app
```

若執行環境的 home cache 不可寫，可改用：

```bash
UV_CACHE_DIR=/tmp/uv-cache uv sync
UV_CACHE_DIR=/tmp/uv-cache uv run python -m web.backend.app
```

後端預設監聽 `http://127.0.0.1:5001`，主要 API：

- `GET /health`
- `GET /get_money_data?state=date-desc`

## 前端啟動

前端是靜態頁面，可直接開啟：

```text
web/frontend/index.html
```

如後端不在 `http://127.0.0.1:5001`，可在載入 `app.js` 前設定：

```html
<script>
    window.API_BASE_URL = "http://你的後端位址:5001";
</script>
```

## 韌體

主程式位於：

```text
firmware/electronic_piggy_bank/electronic_piggy_bank.ino
```

主韌體已拆成幾個 Arduino 分頁：

- `hardware_config.h`：pin 腳、Wi-Fi、NTP、感測閾值等設定
- `camera_server.ino`：ESP32-CAM 初始化與 `/stream` 串流服務
- `display.ino`：OLED 畫面繪製
- `sensors_and_time.ino`：超音波距離、NTP 與本機時間更新
- `actuators.ino`：伺服馬達、蜂鳴器與存錢動作

測試草稿位於：

```text
firmware/examples/
```

Arduino/ESP32 依賴包含 `esp_camera`、`HX711`、`U8g2`、`NTPClient`、`ESP32Servo` 等，請先在 Arduino IDE 或 Arduino CLI 安裝。

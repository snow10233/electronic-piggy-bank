esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t jpgBufferLength;
    uint8_t *jpgBuffer;
    char partBuffer[64];
    bool isJpegConverted;

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if (res != ESP_OK) {
        return res;
    }

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE("CAMERA", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        isJpegConverted = false;

        if (fb->format != PIXFORMAT_JPEG) {
            bool jpegConverted = frame2jpg(fb, 80, &jpgBuffer, &jpgBufferLength);
            isJpegConverted = true;
            esp_camera_fb_return(fb);
            fb = NULL;

            if (!jpegConverted) {
                ESP_LOGE("CAMERA", "JPEG compression failed");
                res = ESP_FAIL;
                break;
            }
        } else {
            jpgBufferLength = fb->len;
            jpgBuffer = fb->buf;
        }

        if (res == ESP_OK) {
            size_t headerLength = snprintf(
                partBuffer,
                64,
                "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n",
                jpgBufferLength
            );
            res = httpd_resp_send_chunk(req, partBuffer, headerLength);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(jpgBuffer), jpgBufferLength);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }

        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        } else if (isJpegConverted && jpgBuffer) {
            free(jpgBuffer);
        }

        if (res != ESP_OK) {
            break;
        }
    }

    return res;
}

void cameraSetup() {
    camera_config_t config;
    config.pin_d0 = HardwareConfig::CAMERA_D0_PIN;
    config.pin_d1 = HardwareConfig::CAMERA_D1_PIN;
    config.pin_d2 = HardwareConfig::CAMERA_D2_PIN;
    config.pin_d3 = HardwareConfig::CAMERA_D3_PIN;
    config.pin_d4 = HardwareConfig::CAMERA_D4_PIN;
    config.pin_d5 = HardwareConfig::CAMERA_D5_PIN;
    config.pin_d6 = HardwareConfig::CAMERA_D6_PIN;
    config.pin_d7 = HardwareConfig::CAMERA_D7_PIN;
    config.pin_xclk = HardwareConfig::CAMERA_XCLK_PIN;
    config.pin_pclk = HardwareConfig::CAMERA_PCLK_PIN;
    config.pin_vsync = HardwareConfig::CAMERA_VSYNC_PIN;
    config.pin_href = HardwareConfig::CAMERA_HREF_PIN;
    config.pin_sccb_sda = HardwareConfig::CAMERA_SIOD_PIN;
    config.pin_sccb_scl = HardwareConfig::CAMERA_SIOC_PIN;
    config.pin_pwdn = HardwareConfig::CAMERA_PWDN_PIN;
    config.pin_reset = HardwareConfig::CAMERA_RESET_PIN;
    config.xclk_freq_hz = HardwareConfig::CAMERA_XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = HardwareConfig::CAMERA_JPEG_QUALITY;
    config.fb_count = HardwareConfig::CAMERA_FB_COUNT;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    startServer();
}

void startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_uri_t streamUri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &streamUri);
    }
}

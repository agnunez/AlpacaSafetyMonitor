/*  AlpacaApp – Front-end logic for Alpaca ESP32 Observatory Setup
 *  Talks to REST endpoints served by the ESP32 firmware.
 */
var AlpacaApp = (function () {
    "use strict";

    /* ── helpers ─────────────────────────────────────────────── */

    function api(method, url, data) {
        var opts = {
            method: method,
            url: url,
            dataType: "json",
            contentType: "application/json",
            timeout: 10000
        };
        if (data !== undefined) opts.data = JSON.stringify(data);
        return $.ajax(opts);
    }

    function showAlert(sel, msg, cls) {
        cls = cls || "info";
        $(sel).html('<div class="alert alert-' + cls + ' py-1 small">' + msg + '</div>');
    }

    function badge(ok, label) {
        var cls = ok ? "badge-success" : "badge-danger";
        return '<span class="badge ' + cls + '">' + label + '</span>';
    }

    function healthBadge(h) {
        var map = { OK: "badge-success", DEGRADED: "badge-warning", FAILED: "badge-danger" };
        return '<span class="badge ' + (map[h] || "badge-secondary") + '">' + h + '</span>';
    }

    /* ── STATUS TAB ──────────────────────────────────────────── */

    function refreshStatus() {
        api("GET", "/api/wifi/status").done(function (d) {
            // piggyback WiFi info on status page if wanted
        });

        // Sensor health
        api("GET", "/api/sensors/health").done(function (d) {
            var html = '<table class="table table-sm table-bordered">';
            html += '<thead><tr><th>Sensor</th><th>Health</th><th>Value</th></tr></thead><tbody>';
            if (d.sensors) {
                $.each(d.sensors, function (i, s) {
                    html += '<tr><td>' + s.name + '</td><td>' + healthBadge(s.health) +
                        '</td><td>' + (s.value !== undefined ? s.value.toFixed(2) : "N/A") + '</td></tr>';
                });
            }
            html += '</tbody></table>';
            $("#sensor-health").html(html);
        }).fail(function () {
            showAlert("#sensor-health", "Failed to load sensor data", "danger");
        });

        // Safety state – read from Alpaca safety monitor endpoint
        api("GET", "/api/safety/state").done(function (d) {
            var html = '<table class="table table-sm table-bordered">';
            html += '<tr><td><strong>Safe</strong></td><td>' + badge(d.safe, d.safe ? "SAFE" : "UNSAFE") + '</td></tr>';
            html += '<tr><td><strong>State</strong></td><td>' + (d.state || "—") + '</td></tr>';
            if (d.metrics) {
                $.each(d.metrics, function (name, m) {
                    html += '<tr><td>' + name + '</td><td>' +
                        badge(m.safe, m.safe ? "safe" : "unsafe") +
                        ' <small>val=' + (m.value !== undefined ? m.value.toFixed(2) : "N/A") + '</small></td></tr>';
                });
            }
            html += '</table>';
            $("#safety-status").html(html);
        }).fail(function () {
            showAlert("#safety-status", "Failed to load safety state", "danger");
        });
    }

    /* ── WIFI TAB ────────────────────────────────────────────── */

    function loadWifiStatus() {
        api("GET", "/api/wifi/status").done(function (d) {
            var html = '<table class="table table-sm table-bordered">';
            html += '<tr><td>Mode</td><td>' + (d.state || "?") + '</td></tr>';
            html += '<tr><td>SSID</td><td>' + (d.ssid || "—") + '</td></tr>';
            html += '<tr><td>IP</td><td>' + (d.ip || "—") + '</td></tr>';
            html += '<tr><td>RSSI</td><td>' + (d.rssi !== undefined ? d.rssi + " dBm" : "—") + '</td></tr>';
            html += '</table>';
            $("#wifi-status").html(html);
        });
    }

    function loadWifiNetworks() {
        api("GET", "/api/wifi/networks").done(function (d) {
            // Endpoint returns a bare array
            var networks = Array.isArray(d) ? d : (d.networks || []);
            if (networks.length === 0) {
                $("#wifi-networks").html('<p class="text-muted small">No stored networks.</p>');
                return;
            }
            var html = '<table class="table table-sm table-bordered">';
            html += '<thead><tr><th>SSID</th><th>Priority</th><th></th></tr></thead><tbody>';
            $.each(networks, function (i, n) {
                html += '<tr><td>' + n.ssid + '</td><td>' + n.priority + '</td>';
                html += '<td><button class="btn btn-danger btn-sm py-0" onclick="AlpacaApp.deleteWifi(' +
                    i + ')">Del</button></td></tr>';
            });
            html += '</tbody></table>';
            $("#wifi-networks").html(html);
        });
    }

    function scanWifi() {
        $("#wifi-scan").html('<p class="text-muted small">Scanning...</p>');
        // Kick off async scan, then poll for results
        api("GET", "/api/wifi/scan").done(function (d) {
            if (d.scanning) {
                // Scan in progress, poll until done
                var pollCount = 0;
                var pollInterval = setInterval(function () {
                    pollCount++;
                    if (pollCount > 20) { // max ~10s
                        clearInterval(pollInterval);
                        $("#wifi-scan").html('<p class="text-muted small">Scan timed out. Try again.</p>');
                        return;
                    }
                    api("GET", "/api/wifi/scan").done(function (r) {
                        if (!r.scanning) {
                            clearInterval(pollInterval);
                            renderScanResults(r.networks || []);
                        }
                    }).fail(function () {
                        clearInterval(pollInterval);
                        showAlert("#wifi-scan", "Scan failed", "danger");
                    });
                }, 500);
            } else {
                renderScanResults(d.networks || []);
            }
        }).fail(function () {
            showAlert("#wifi-scan", "Scan failed", "danger");
        });
    }

    function renderScanResults(networks) {
        if (networks.length === 0) {
            $("#wifi-scan").html('<p class="text-muted small">No networks found.</p>');
            return;
        }
        var html = '<table class="table table-sm table-bordered">';
        html += '<thead><tr><th>SSID</th><th>RSSI</th><th>Enc</th><th></th></tr></thead><tbody>';
        $.each(networks, function (i, n) {
            html += '<tr><td>' + n.ssid + '</td><td>' + n.rssi + '</td><td>' +
                (n.open ? "Open" : "Yes") + '</td>';
            html += '<td><button class="btn btn-primary btn-sm py-0" onclick="$(\'#wifi-ssid\').val(\'' +
                n.ssid.replace(/'/g, "\\'") + '\')">Select</button></td></tr>';
        });
        html += '</tbody></table>';
        $("#wifi-scan").html(html);
    }

    function connectWifi() {
        var ssid = $("#wifi-ssid").val().trim();
        var pass = $("#wifi-pass").val();
        if (!ssid) { alert("Enter SSID"); return; }
        api("POST", "/api/wifi/connect", { ssid: ssid, password: pass }).done(function (d) {
            showAlert("#wifi-scan", "Connecting to " + ssid + "...", "success");
            setTimeout(function () { loadWifiStatus(); loadWifiNetworks(); }, 5000);
        }).fail(function () {
            showAlert("#wifi-scan", "Connect request failed", "danger");
        });
    }

    function deleteWifi(index) {
        api("POST", "/api/wifi/delete", { index: index }).done(function () {
            loadWifiNetworks();
        });
    }

    function startAP() {
        api("POST", "/api/wifi/ap").done(function () {
            showAlert("#wifi-scan", "AP mode started. Connect to the ESP32 AP.", "warning");
        });
    }

    /* ── DEVICES TAB (Alpaca jsonFormer) ─────────────────────── */

    function loadDevices() {
        api("GET", "/management/v1/configureddevices").done(function (d) {
            var devices = d.Value || [];
            var links = "";
            var firstId = null;
            $.each(devices, function (i, dev) {
                var id = "dev-" + dev.DeviceType + "-" + dev.DeviceNumber;
                if (i === 0) firstId = id;
                links += '<li class="nav-item"><a class="nav-link' + (i === 0 ? " active" : "") +
                    '" data-toggle="tab" href="#' + id + '">' +
                    dev.DeviceName + ' (' + dev.DeviceType + ')' + '</a></li>';
            });
            $("#device-links").html(links);

            var forms = "";
            $.each(devices, function (i, dev) {
                var id = "dev-" + dev.DeviceType + "-" + dev.DeviceNumber;
                forms += '<div class="tab-pane fade' + (i === 0 ? " show active" : "") +
                    '" id="' + id + '" data-device-type="' + dev.DeviceType +
                    '" data-device-number="' + dev.DeviceNumber + '">';
                forms += '<div class="jf-container"></div></div>';
            });
            $("#device-form-container").html(forms);

            // Load each device's jsonFormer form
            $.each(devices, function (i, dev) {
                var id = "dev-" + dev.DeviceType + "-" + dev.DeviceNumber;
                var url = "/api/v1/" + dev.DeviceType.toLowerCase() + "/" +
                    dev.DeviceNumber + "/jsondata";
                api("GET", url).done(function (schema) {
                    $("#" + id + " .jf-container").jsonFormer(schema);
                });
            });
        });
    }

    function initDeviceButtons() {
        $("#device-refresh").click(function () { loadDevices(); });
        $("#device-update").click(function () {
            var pane = $("#device-form-container .tab-pane.active");
            var type = pane.data("device-type");
            var num = pane.data("device-number");
            var data = pane.find(".jf-container").jsonFormer("value");
            var url = "/api/v1/" + type.toLowerCase() + "/" + num + "/jsondata";
            api("PUT", url, data).done(function () {
                showAlert("#device-form-container", "Updated (not saved to flash)", "success");
            }).fail(function () {
                showAlert("#device-form-container", "Update failed", "danger");
            });
        });
        $("#device-save").click(function () {
            api("PUT", "/management/v1/server/save").done(function () {
                showAlert("#device-form-container", "Saved to flash", "success");
            }).fail(function () {
                showAlert("#device-form-container", "Save failed", "danger");
            });
        });
    }

    /* ── HARDWARE TAB ────────────────────────────────────────── */

    function loadHardware() {
        api("GET", "/api/hardware").done(function (d) {
            $("#hw-hostname").val(d.hostname || "");
            $("#hw-sda").val(d.i2c_sda);
            $("#hw-scl").val(d.i2c_scl);
            $("#hw-bme-addr").val(d.bme280_addr ? "0x" + d.bme280_addr.toString(16).toUpperCase() : "");
            $("#hw-mlx-addr").val(d.mlx90614_addr ? "0x" + d.mlx90614_addr.toString(16).toUpperCase() : "");
            $("#hw-led").val(d.wifi_led_pin);
            $("#hw-interval").val(d.measure_interval_ms);
        }).fail(function () {
            showAlert("#hw-form", "Failed to load hardware settings", "danger");
        });
    }

    function saveHardware() {
        var data = {
            hostname: $("#hw-hostname").val().trim(),
            i2c_sda: parseInt($("#hw-sda").val(), 10),
            i2c_scl: parseInt($("#hw-scl").val(), 10),
            bme280_addr: parseInt($("#hw-bme-addr").val(), 16),
            mlx90614_addr: parseInt($("#hw-mlx-addr").val(), 16),
            wifi_led_pin: parseInt($("#hw-led").val(), 10),
            measure_interval_ms: parseInt($("#hw-interval").val(), 10)
        };
        api("POST", "/api/hardware", data).done(function () {
            showAlert("#hw-form", "Hardware settings saved. Reboot to apply I2C changes.", "success");
        }).fail(function () {
            showAlert("#hw-form", "Save failed", "danger");
        });
    }

    /* ── SAFETY TAB ──────────────────────────────────────────── */

    function loadSafety() {
        api("GET", "/api/safety").done(function (d) {
            var html = '';
            var metrics = ["temperature", "sky_temp", "humidity", "dew_margin"];
            var labels = { temperature: "Ambient Temp", sky_temp: "Sky Temp", humidity: "Humidity", dew_margin: "Dew Margin" };

            $.each(metrics, function (i, key) {
                var t = d[key] || {};
                html += '<div class="card mb-2"><div class="card-body py-2">';
                html += '<h6>' + (labels[key] || key) + '</h6>';
                html += '<div class="form-group row mb-1">';
                html += '<label class="col-sm-4 col-form-label col-form-label-sm">Enabled</label>';
                html += '<div class="col-sm-8"><div class="custom-control custom-switch mt-1">';
                html += '<input type="checkbox" class="custom-control-input" id="sf-' + key + '-en"' +
                    (t.enabled ? ' checked' : '') + '>';
                html += '<label class="custom-control-label" for="sf-' + key + '-en">Enable</label>';
                html += '</div></div></div>';
                html += row("sf-" + key + "-arm", "Arm (safe)", t.arm);
                html += row("sf-" + key + "-disarm", "Disarm (unsafe)", t.disarm);
                html += row("sf-" + key + "-weight", "Weight", t.weight);
                html += '</div></div>';
            });

            html += '<hr>';
            html += row("sf-delay-open", "Delay to Open (s)", d.delay_to_open_s);
            html += row("sf-delay-close", "Delay to Close (s)", d.delay_to_close_s);
            html += row("sf-trend-window", "Trend Window (s)", d.trend_window_s);
            html += row("sf-trend-thresh", "Trend Threshold", d.trend_threshold);

            $("#safety-form").html(html);
        }).fail(function () {
            showAlert("#safety-form", "Failed to load safety settings", "danger");
        });
    }

    function row(id, label, val) {
        return '<div class="form-group row mb-1">' +
            '<label class="col-sm-4 col-form-label col-form-label-sm">' + label + '</label>' +
            '<div class="col-sm-8"><input type="number" step="any" class="form-control form-control-sm" id="' +
            id + '" value="' + (val !== undefined ? val : "") + '"></div></div>';
    }

    function saveSafety() {
        var metrics = ["temperature", "sky_temp", "humidity", "dew_margin"];
        var data = {};
        $.each(metrics, function (i, key) {
            data[key] = {
                enabled: $("#sf-" + key + "-en").is(":checked"),
                arm: parseFloat($("#sf-" + key + "-arm").val()),
                disarm: parseFloat($("#sf-" + key + "-disarm").val()),
                weight: parseFloat($("#sf-" + key + "-weight").val())
            };
        });
        data.delay_to_open_s = parseFloat($("#sf-delay-open").val());
        data.delay_to_close_s = parseFloat($("#sf-delay-close").val());
        data.trend_window_s = parseFloat($("#sf-trend-window").val());
        data.trend_threshold = parseFloat($("#sf-trend-thresh").val());

        api("POST", "/api/safety", data).done(function () {
            showAlert("#safety-form", "Safety settings saved", "success");
        }).fail(function () {
            showAlert("#safety-form", "Save failed", "danger");
        });
    }

    /* ── DOME TAB ────────────────────────────────────────────── */

    var _domePollTimer = null;
    var _domeTxn = 1;

    function loadDome() {
        api("GET", "/api/dome").done(function (d) {
            $("#dome-mo").val(d.motor_open_pin);
            $("#dome-mc").val(d.motor_close_pin);
            $("#dome-mp").val(d.motor_power_pin);
            $("#dome-eo-pin").val(d.endstop_open_pin);
            $("#dome-ec-pin").val(d.endstop_close_pin);
            $("#dome-actlow").prop("checked", !!d.endstop_active_low);
            $("#dome-tmove").val(d.move_timeout_ms);
            $("#dome-trev").val(d.reversal_ms);
            $("#dome-tdeb").val(d.debounce_ms);
            $("#dome-tretry").val(d.retry_pause_ms);
            $("#dome-override").prop("checked", !!d.safety_override);
            updateDomeStatus(d);
        }).fail(function () {
            showAlert("#dome-form", "Failed to load dome settings", "danger");
        });
    }

    function updateDomeStatus(d) {
        var names = ["Open", "Closed", "Opening", "Closing", "Error"];
        var s = (typeof d.shutter_status === "number") ? d.shutter_status : -1;
        var name = (s >= 0 && s <= 4) ? names[s] : (d.shutter_name || "Unknown");
        $("#dome-state").text(name);
        $("#dome-eo").html(badge(!!d.endstop_open, d.endstop_open ? "TRIGGERED" : "open"));
        $("#dome-ec").html(badge(!!d.endstop_closed, d.endstop_closed ? "TRIGGERED" : "open"));
    }

    function refreshDomeStatus() {
        api("GET", "/api/dome").done(function (d) {
            updateDomeStatus(d);
        });
    }

    function saveDome() {
        var data = {
            motor_open_pin: parseInt($("#dome-mo").val(), 10),
            motor_close_pin: parseInt($("#dome-mc").val(), 10),
            motor_power_pin: parseInt($("#dome-mp").val(), 10),
            endstop_open_pin: parseInt($("#dome-eo-pin").val(), 10),
            endstop_close_pin: parseInt($("#dome-ec-pin").val(), 10),
            endstop_active_low: $("#dome-actlow").is(":checked"),
            move_timeout_ms: parseInt($("#dome-tmove").val(), 10),
            reversal_ms: parseInt($("#dome-trev").val(), 10),
            debounce_ms: parseInt($("#dome-tdeb").val(), 10),
            retry_pause_ms: parseInt($("#dome-tretry").val(), 10),
            safety_override: $("#dome-override").is(":checked")
        };
        api("POST", "/api/dome", data).done(function (r) {
            if (r && r.reboot_required) {
                showAlert("#dome-form", "Dome settings saved. Reboot required for pin changes to take effect.", "warning");
            } else {
                showAlert("#dome-form", "Dome settings saved", "success");
            }
        }).fail(function () {
            showAlert("#dome-form", "Save failed", "danger");
        });
    }

    function _domeAlpacaPut(action) {
        var fd = new URLSearchParams();
        fd.append("ClientID", "1");
        fd.append("ClientTransactionID", String(_domeTxn++));
        return $.ajax({
            url: "/api/v1/dome/0/" + action,
            method: "PUT",
            data: fd.toString(),
            contentType: "application/x-www-form-urlencoded",
            timeout: 10000
        });
    }

    function domeOpen() {
        _domeAlpacaPut("openshutter").done(function (r) {
            if (r && r.ErrorNumber) {
                showAlert("#dome-form", "Open rejected: " + (r.ErrorMessage || "error"), "warning");
            } else {
                showAlert("#dome-form", "Open command sent", "success");
            }
            refreshDomeStatus();
        }).fail(function () {
            showAlert("#dome-form", "Open command failed", "danger");
        });
    }

    function domeClose() {
        _domeAlpacaPut("closeshutter").done(function () {
            showAlert("#dome-form", "Close command sent", "success");
            refreshDomeStatus();
        }).fail(function () {
            showAlert("#dome-form", "Close command failed", "danger");
        });
    }

    function domeAbort() {
        _domeAlpacaPut("abortslew").done(function () {
            showAlert("#dome-form", "Abort sent", "warning");
            refreshDomeStatus();
        }).fail(function () {
            showAlert("#dome-form", "Abort failed", "danger");
        });
    }

    function startDomePoll() {
        stopDomePoll();
        _domePollTimer = setInterval(refreshDomeStatus, 5000);
    }

    function stopDomePoll() {
        if (_domePollTimer) {
            clearInterval(_domePollTimer);
            _domePollTimer = null;
        }
    }

    /* ── MQTT TAB ────────────────────────────────────────────── */

    function loadMqtt() {
        api("GET", "/api/mqtt").done(function (d) {
            $("#mqtt-enabled").prop("checked", d.enabled);
            $("#mqtt-broker").val(d.broker || "");
            $("#mqtt-port").val(d.port || 1883);
            $("#mqtt-user").val(d.username || "");
            $("#mqtt-pass").val(d.password || "");
            $("#mqtt-topic").val(d.topic_prefix || "");
            $("#mqtt-interval").val(d.publish_interval_ms);
        }).fail(function () {
            showAlert("#mqtt-form", "Failed to load MQTT settings", "danger");
        });
    }

    function saveMqtt() {
        var data = {
            enabled: $("#mqtt-enabled").is(":checked"),
            broker: $("#mqtt-broker").val().trim(),
            port: parseInt($("#mqtt-port").val(), 10),
            username: $("#mqtt-user").val(),
            password: $("#mqtt-pass").val(),
            topic_prefix: $("#mqtt-topic").val().trim(),
            publish_interval_ms: parseInt($("#mqtt-interval").val(), 10)
        };
        api("POST", "/api/mqtt", data).done(function () {
            showAlert("#mqtt-form", "MQTT settings saved", "success");
        }).fail(function () {
            showAlert("#mqtt-form", "Save failed", "danger");
        });
    }

    /* ── OTA TAB ─────────────────────────────────────────────── */

    function loadVersion() {
        api("GET", "/api/ota/version").done(function (d) {
            $("#ota-version").text(d.version || "unknown");
        });
    }

    function uploadFirmware() {
        var fileInput = document.getElementById("ota-file");
        if (!fileInput.files.length) { alert("Select a .bin file first"); return; }

        var file = fileInput.files[0];
        if (!file.name.endsWith(".bin")) { alert("File must be a .bin firmware image"); return; }

        var formData = new FormData();
        formData.append("firmware", file);

        var $bar = $("#ota-progress-bar");
        var $progress = $bar.find(".progress-bar");
        $bar.show();
        $progress.css("width", "0%").text("0%");
        $("#ota-status").html("");

        $.ajax({
            url: "/api/ota/upload",
            type: "POST",
            data: formData,
            processData: false,
            contentType: false,
            timeout: 120000,
            xhr: function () {
                var xhr = new XMLHttpRequest();
                xhr.upload.addEventListener("progress", function (e) {
                    if (e.lengthComputable) {
                        var pct = Math.round(e.loaded / e.total * 100);
                        $progress.css("width", pct + "%").text(pct + "%");
                    }
                });
                return xhr;
            },
            success: function (d) {
                $progress.css("width", "100%").text("100%");
                showAlert("#ota-status", "Firmware uploaded. Rebooting...", "success");
                setTimeout(function () { location.reload(); }, 10000);
            },
            error: function (xhr) {
                var msg = "Upload failed";
                try { msg = JSON.parse(xhr.responseText).error || msg; } catch (e) { }
                showAlert("#ota-status", msg, "danger");
            }
        });
    }

    /* ── BACKUP TAB ──────────────────────────────────────────── */

    function exportSettings() {
        window.location.href = "/api/settings/export";
    }

    function importSettings() {
        var fileInput = document.getElementById("import-file");
        if (!fileInput.files.length) { alert("Select a JSON file first"); return; }

        var reader = new FileReader();
        reader.onload = function (e) {
            var text = e.target.result;
            var json;
            try { json = JSON.parse(text); } catch (err) {
                showAlert("#backup-status", "Invalid JSON file", "danger");
                return;
            }
            api("POST", "/api/settings/import", json).done(function (d) {
                if (d.success) {
                    showAlert("#backup-status", "Settings imported successfully. Reboot recommended.", "success");
                } else {
                    showAlert("#backup-status", "Import failed: " + (d.error || "unknown"), "danger");
                }
            }).fail(function () {
                showAlert("#backup-status", "Import request failed", "danger");
            });
        };
        reader.readAsText(fileInput.files[0]);
    }

    /* ── INIT ────────────────────────────────────────────────── */

    function init() {
        // Load default data for first visible tab
        refreshStatus();

        // Tab change handlers – lazy load data
        $('a[data-toggle="tab"]').on("shown.bs.tab", function (e) {
            var target = $(e.target).attr("href");
            // Stop dome polling whenever leaving the dome tab
            stopDomePoll();
            switch (target) {
                case "#tab-status":  refreshStatus(); break;
                case "#tab-wifi":    loadWifiStatus(); loadWifiNetworks(); break;
                case "#tab-devices": loadDevices(); break;
                case "#tab-hardware": loadHardware(); break;
                case "#tab-safety":  loadSafety(); break;
                case "#tab-dome":    loadDome(); startDomePoll(); break;
                case "#tab-mqtt":    loadMqtt(); break;
                case "#tab-ota":     loadVersion(); break;
            }
        });

        initDeviceButtons();
    }

    $(document).ready(init);

    /* ── PUBLIC API ──────────────────────────────────────────── */

    return {
        refreshStatus: refreshStatus,
        scanWifi: scanWifi,
        connectWifi: connectWifi,
        deleteWifi: deleteWifi,
        startAP: startAP,
        loadHardware: loadHardware,
        saveHardware: saveHardware,
        loadSafety: loadSafety,
        saveSafety: saveSafety,
        loadDome: loadDome,
        saveDome: saveDome,
        domeOpen: domeOpen,
        domeClose: domeClose,
        domeAbort: domeAbort,
        refreshDomeStatus: refreshDomeStatus,
        loadMqtt: loadMqtt,
        saveMqtt: saveMqtt,
        uploadFirmware: uploadFirmware,
        exportSettings: exportSettings,
        importSettings: importSettings,
        loadVersion: loadVersion
    };

})();

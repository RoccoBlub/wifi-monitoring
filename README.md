# WiFi Monitoring System using InfluxDB & Grafana on Raspberry Pi

This project sets up a WiFi monitoring system on a Raspberry Pi. It logs WiFi signal strength and connectivity status
every 30 seconds, storing data in InfluxDB, and visualizing it using Grafana.

The system is designed to auto-restart using systemd, ensuring that the monitoring script runs continuously, even after
reboots.

## 📌 Features

- ✅ Logs WiFi signal strength (dBm) and connectivity status (Connected/Disconnected)
- ✅ Stores data in InfluxDB for easy querying
- ✅ Visualizes data in Grafana
- ✅ Auto-restarts if it crashes using systemd
- ✅ Supports multiple locations (e.g., "Living Room", "Bedroom")

## 📜 Installation Steps

Follow these steps to set up the WiFi monitoring system on your Raspberry Pi.

### 1️⃣ Update & Upgrade the System

Before installing any software, make sure your Raspberry Pi is up to date:

```shell
sudo apt update && sudo apt full-upgrade -y
sudo reboot
```

### 2️⃣ Install Dependencies

Install required tools:

```shell
sudo apt install -y curl git gcc make
```

### 3️⃣ Install InfluxDB

Install and enable InfluxDB (v2.x) for storing WiFi data.

```shell
wget -qO- https://repos.influxdata.com/influxdata-archive_compat.key | sudo tee /etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg > /dev/null
echo "deb https://repos.influxdata.com/debian stable main" | sudo tee /etc/apt/sources.list.d/influxdb.list
sudo apt update
sudo apt install -y influxdb
sudo systemctl enable --now influxdb
```

Check if InfluxDB is running if not start it manually:

```shell
sudo systemctl status influxdb
sudo systemctl start influxdb # Starting it manually
```

### 4️⃣ Set Up InfluxDB

Run the InfluxDB setup wizard:

```shell
influx setup
```

You will be asked for:

- **Username:** `admin`
- **Password:** Choose a secure password
- **Organization Name:** `wifi_monitoring_org`
- **Bucket Name:** `wifi_monitoring`
- **Retention Policy:** `0` (infinite retention)
- **Initial Admin Token:** Copy and **save** this token, you will need it later.

Once completed, verify the setup:

```shell
influx bucket list
influx org list
```

### 5️⃣ Install & Configure Grafana

Install Grafana for dashboard visualization:

```shell
sudo apt install -y grafana
sudo systemctl enable --now grafana-server
```

Check if Grafana is running if not start it manually:

```shell
sudo systemctl status grafana-server
sudo systemctl start grafana-server # Starting it manually
```

Grafana runs on port 3000. Open it in your browser:

```shell
http://<your-raspberry-pi-ip>:3000
```

Default login:

- **Username:** `admin`
- **password:** `admin` (you will be asked to change it)

### 6️⃣ Compile & Run the WiFi Monitoring Script

Clone the repository and compile the C program:

```shell
git clone <your-repo-url>
cd wifi-monitoring
gcc wifi_monitor.c -o wifi_monitor
```

Run it manually for testing:

```shell
./wifi_monitor "Living Room" # Replace "Living Room" with your current location.
```

Check InfluxDB logs to see if data is being inserted:

```shell
influx query 'from(bucket: "wifi_monitoring") |> range(start: -1h)'
```

### 7️⃣ Set Up Auto-Restart with systemd

Create a systemd service to ensure the script restarts if it crashes and runs on boot.
run:

```shell
sudo nano /etc/systemd/system/wifi_monitor.service
```

Paste the following:

```ini
[Unit]
Description = WiFi Monitoring Script
After = network.target

[Service]
ExecStart = /home/pi/wifi-monitoring/wifi_monitor "Living Room"
Restart = always
RestartSec = 10
StandardOutput = append:/home/pi/wifi_monitor.log
StandardError = append:/home/pi/wifi_monitor.log

[Install]
WantedBy = multi-user.target
```

Save (CTRL + X, Y, Enter).

Enable & Start the Service:

```shell
sudo systemctl daemon-reload
sudo systemctl enable wifi_monitor.service
sudo systemctl start wifi_monitor.service
```

Check the status and check logs if needed:

```shell
sudo systemctl status wifi_monitor.service
journalctl -u wifi_monitor.service --follow # logs
```

## 📊 Setting Up the Grafana Dashboard

1. Open Grafana in your browser: `http://<your-raspberry-pi-ip>:3000`
2. Log in with admin credentials.
3. Add an InfluxDB Data Source:
    - Go to Settings → Data Sources → Add new data source
    - Select InfluxDB
    - Set:
        - URL: `http://localhost:8086`
        - Database: `wifi_monitoring`
        - Token: (Use the token from Step 4)
        - Organization: `wifi_monitoring_org`
    - Click Save & Test (Ensure it works!)
4. Create a Dashboard:
    - Go to Create → Dashboard → Add New Panel
    - Use this query:
   ```shell
   from(bucket: "wifi_monitoring")
    |> range(start: -24h)
    |> filter(fn: (r) => r._measurement == "wifi_status")
    |> filter(fn: (r) => r._field == "strength")
   ```
    - Choose Line Chart visualization.
    - Click Apply and save the dashboard.

## ✅ Usage & Troubleshooting

### 🔹 Restart or Stop the Service
```shell
sudo systemctl restart wifi_monitor.service
sudo systemctl stop wifi_monitor.service
```

### 🔹 View Logs
```shell
journalctl -u wifi_monitor.service --follow
```

### 🔹 Check if the Monitoring Script is Running
```shell
ps aux | grep wifi_monitor
```

### 🔹 Manually Insert Test Data into InfluxDB
```shell
curl -i -XPOST 'http://localhost:8086/api/v2/write?org=wifi_monitoring_org&bucket=wifi_monitoring&precision=s' \
--header 'Authorization: Token YOUR_TOKEN' \
--header 'Content-Type: text/plain' \
--data-binary 'wifi_status,location="Living Room" status="Connected",strength=-45 1640995200'
```

## 🛠️ Possible Issues & Fixes

### ❌ Wi-Fi Not Being Logged
Run:
```shell
iwconfig wlan0
```
if you see "**No wireless extensions**", the Wi-Fi module might be disabled.

### ❌ InfluxDB Not Accepting Data
Ensure its running and restart if needed:
```shell
sudo systemctl status influxdb
sudo systemctl restart influxdb # restart
```

### ❌ Grafana Shows "No Data"
Check InfluxDB logs:
```shell
influx query 'from(bucket: "wifi_monitoring") |> range(start: -1h)'
```
If there's no data, your script might not be running.
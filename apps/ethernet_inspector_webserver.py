#!/usr/bin/env python3
import threading
import time
from collections import deque
import zmq
import pmt
from flask import Flask, render_template_string, jsonify

ZMQ_ENDPOINT = "tcp://127.0.0.1:5555"
MAX_FRAMES = 500

frames = deque(maxlen=MAX_FRAMES)
frame_counter = 0
lock = threading.Lock()

def get_str(d, key, default=""):
    try:
        val = pmt.dict_ref(d, pmt.intern(key), pmt.PMT_NIL)
        if pmt.is_symbol(val):
            return pmt.symbol_to_string(val)
    except Exception:
        pass
    return default

def get_long(d, key, default=-1):
    try:
        val = pmt.dict_ref(d, pmt.intern(key), pmt.PMT_NIL)
        if pmt.is_integer(val):
            return pmt.to_long(val)
    except Exception:
        pass
    return default

def get_bool(d, key, default=False):
    try:
        val = pmt.dict_ref(d, pmt.intern(key), pmt.PMT_NIL)
        if pmt.is_bool(val):
            return pmt.to_bool(val)
    except Exception:
        pass
    return default

def zmq_receiver():
    global frame_counter
    context = zmq.Context()
    sock = context.socket(zmq.SUB)
    sock.connect(ZMQ_ENDPOINT)
    sock.setsockopt_string(zmq.SUBSCRIBE, "")

    print(f"ZMQ connected to {ZMQ_ENDPOINT}")

    while True:
        try:
            msg = sock.recv()
        except Exception as e:
            print(f"ZMQ recv error: {e}")
            time.sleep(1)
            continue

        try:
            d = pmt.deserialize_str(msg)
        except Exception as e:
            print(f"PMT deserialize error: {e}")
            continue

        if not pmt.is_dict(d):
            continue

        mac_src = get_str(d, "mac_src")
        mac_dst = get_str(d, "mac_dst")
        eth = get_long(d, "ethertype", 0)
        eth_name = get_str(d, "ethertype_name", "")

        ip_version = get_long(d, "ip_version", 0)
        ip_src = get_str(d, "ip_src")
        ip_dst = get_str(d, "ip_dst")
        l4_proto = get_long(d, "l4_proto", -1)
        l4_name = get_str(d, "l4_name")
        src_port = get_long(d, "src_port", -1)
        dst_port = get_long(d, "dst_port", -1)
        icmp_type = get_long(d, "icmp_type", -1)
        icmp_code = get_long(d, "icmp_code", -1)
        
        frame_len = get_long(d, "frame_length", -1)
        ip_ttl = get_long(d, "ip_ttl", -1)
        tcp_flags = get_str(d, "tcp_flags", "")
        payload_preview = get_str(d, "payload_preview", "")

        info = get_str(d, "info")

        if eth_name.startswith("IPv") and l4_name:
            proto_label = f"{eth_name}/{l4_name}"
        else:
            proto_label = eth_name

        with lock:
            frame_counter += 1
            frames.appendleft({
                "id": frame_counter,
                "timestamp": time.strftime("%H:%M:%S"),
                "mac_src": mac_src,
                "mac_dst": mac_dst,
                "ethertype": f"0x{eth:04x}",
                "ethertype_name": eth_name,
                "frame_len": frame_len,
                "ip_version": ip_version,
                "ip_src": ip_src,
                "ip_dst": ip_dst,
                "ip_ttl": ip_ttl,
                "l4_proto": l4_proto,
                "l4_name": l4_name,
                "src_port": src_port,
                "dst_port": dst_port,
                "tcp_flags": tcp_flags,
                "icmp_type": icmp_type,
                "icmp_code": icmp_code,
                "payload_preview": payload_preview,
                "info": info,
                "proto_label": proto_label,
            })

app = Flask(__name__)

HTML_TEMPLATE = """
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<title>Inspecteur Ethernet</title>
<style>
body {
    font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    margin: 0; padding: 0;
    background: #0b1020; color: #e0e4f0;
}
header {
    padding: 12px 24px;
    background: linear-gradient(90deg, #1b2138, #232b4a);
    box-shadow: 0 2px 4px rgba(0,0,0,0.5);
    display: flex; align-items: center; justify-content: space-between;
}
header h1 { margin: 0; font-size: 1.2rem; flex: 1; }
.header-controls {
    display: flex;
    align-items: center;
    gap: 16px;
}
.frame-count {
    font-size: 0.85rem;
    opacity: 0.8;
}
.clear-btn {
    padding: 6px 16px;
    background: #da3633;
    color: white;
    border: none;
    border-radius: 6px;
    cursor: pointer;
    font-size: 0.85rem;
    font-weight: 500;
    transition: background 0.2s;
}
.clear-btn:hover {
    background: #f85149;
}
.clear-btn:active {
    background: #b62324;
}
main { padding: 16px 24px 32px 24px; }
table {
    width: 100%; border-collapse: collapse;
    background: #151a2b;
    border-radius: 8px;
    overflow: hidden;
    box-shadow: 0 0 0 1px #252c45;
}
thead { background: #202744; }
th, td {
    padding: 6px 10px; font-size: 0.85rem;
    border-bottom: 1px solid #252c45;
    text-align: left; white-space: nowrap;
}
th { font-weight: 600; color: #c3c8e0; }
.summary-row:nth-child(even) { background: #121727; }
.summary-row:hover { background: #263056; cursor: pointer; }
.col-id { width: 50px; }
.col-time { width: 70px; }
.col-len { width: 60px; }
.col-proto { width: 120px; }
.badge {
    display: inline-block; padding: 2px 6px;
    border-radius: 999px; font-size: 0.75rem;
    background: #28375f;
}
.badge-ipv4 { background: #1f6feb; }
.badge-ipv6 { background: #2ea043; }
.badge-arp  { background: #f1a020; }
.badge-tcp  { background: #a371f7; }
.badge-udp  { background: #56d364; }
.badge-icmp { background: #ff7b72; }
.badge-unk  { background: #6e7681; }
.mac { font-family: "Fira Code", monospace; font-size: 0.8rem; }
.details-row td {
    background: #101426;
    border-bottom: 1px solid #252c45;
    font-size: 0.8rem;
}
.details-box {
    padding: 8px 6px;
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 8px 24px;
}
.details-title {
    font-weight: 600; font-size: 0.85rem;
    color: #c3c8e0; margin-bottom: 6px;
    border-bottom: 1px solid #252c45;
    padding-bottom: 2px;
}
.details-item span {
    display: block;
    font-size: 0.78rem;
    padding: 2px 0;
}
.details-label {
    opacity: 0.7;
    display: inline-block;
    min-width: 100px;
}
.payload-preview {
    font-family: "Fira Code", monospace;
    background: #0d1117;
    padding: 4px 8px;
    border-radius: 4px;
    font-size: 0.75rem;
    color: #79c0ff;
    margin-top: 4px;
    overflow-x: auto;
    white-space: pre;
}
.tcp-flags {
    font-family: "Fira Code", monospace;
    color: #ffa657;
}
footer {
    margin-top: 12px;
    font-size: 0.75rem;
    color: #8b92b0;
}
</style>
<script>
let openDetails = new Set();

function toggleDetails(id) {
    const row = document.getElementById("details-" + id);
    if (!row) return;
    if (row.style.display === "none" || row.style.display === "") {
        row.style.display = "table-row";
        openDetails.add(id);
    } else {
        row.style.display = "none";
        openDetails.delete(id);
    }
}

function clearFrames() {
    if (!confirm("Effacer toutes les trames affichées ?")) {
        return;
    }
    
    fetch("/clear", { method: "POST" })
      .then(resp => resp.json())
      .then(data => {
          if (data.success) {
              openDetails.clear();
              fetchFrames();
          }
      })
      .catch(err => console.error("Erreur clear:", err));
}

function renderFrames(frames) {
    const tbody = document.getElementById("frames-body");
    tbody.innerHTML = "";

    for (const f of frames) {
        const id = f.id;

        let badgeClass = "badge-unk";
        if (f.l4_name === "TCP") badgeClass = "badge-tcp";
        else if (f.l4_name === "UDP") badgeClass = "badge-udp";
        else if (f.l4_name === "ICMP") badgeClass = "badge-icmp";
        else if (f.ethertype_name.indexOf("IPv4") !== -1) badgeClass = "badge-ipv4";
        else if (f.ethertype_name.indexOf("IPv6") !== -1) badgeClass = "badge-ipv6";
        else if (f.ethertype_name.indexOf("ARP") !== -1) badgeClass = "badge-arp";

        const tr = document.createElement("tr");
        tr.className = "summary-row";
        tr.onclick = () => toggleDetails(id);

        const frameLen = f.frame_len > 0 ? f.frame_len : "?";

        tr.innerHTML = `
          <td class="col-id">${id}</td>
          <td class="col-time">${f.timestamp}</td>
          <td class="mac">${f.mac_src}</td>
          <td class="mac">${f.mac_dst}</td>
          <td class="col-len">${frameLen}</td>
          <td class="col-proto"><span class="badge ${badgeClass}">${f.proto_label}</span></td>
          <td>${f.info || ""}</td>
        `;

        const trDetails = document.createElement("tr");
        trDetails.id = "details-" + id;
        trDetails.className = "details-row";
        trDetails.style.display = openDetails.has(id) ? "table-row" : "none";

        let ipBlock = "";
        if (f.ip_version === 4) {
            const ttl = f.ip_ttl >= 0 ? `<span><span class="details-label">TTL :</span> ${f.ip_ttl}</span>` : "";
            ipBlock = `
              <span><span class="details-label">Version :</span> IPv4</span>
              <span><span class="details-label">Source :</span> ${f.ip_src}</span>
              <span><span class="details-label">Destination :</span> ${f.ip_dst}</span>
              ${ttl}`;
        } else if (f.ip_version === 6) {
            ipBlock = `
              <span><span class="details-label">Version :</span> IPv6</span>
              <span><span class="details-label">Source :</span> ${f.ip_src}</span>
              <span><span class="details-label">Destination :</span> ${f.ip_dst}</span>`;
        } else {
            ipBlock = `<span>Pas d'en-tête IP</span>`;
        }

        let l4Block = "";
        if (f.l4_name) {
            l4Block += `<span><span class="details-label">Protocole :</span> ${f.l4_name}</span>`;
            if (f.src_port >= 0) {
                l4Block += `<span><span class="details-label">Port source :</span> ${f.src_port}</span>`;
                l4Block += `<span><span class="details-label">Port dest :</span> ${f.dst_port}</span>`;
            }
            if (f.tcp_flags) {
                l4Block += `<span><span class="details-label">Flags TCP :</span> <span class="tcp-flags">${f.tcp_flags}</span></span>`;
            }
            if (f.icmp_type >= 0) {
                l4Block += `<span><span class="details-label">ICMP type :</span> ${f.icmp_type}</span>`;
                l4Block += `<span><span class="details-label">ICMP code :</span> ${f.icmp_code}</span>`;
            }
        } else {
            l4Block = `<span>Pas de protocole L4</span>`;
        }

        let payloadBlock = "";
        if (f.payload_preview) {
            payloadBlock = `<div class="payload-preview">${f.payload_preview}</div>`;
        }

        trDetails.innerHTML = `
          <td colspan="7">
            <div class="details-box">
              <div class="details-item">
                <div class="details-title">Ethernet</div>
                <span><span class="details-label">MAC source :</span> <span class="mac">${f.mac_src}</span></span>
                <span><span class="details-label">MAC dest :</span> <span class="mac">${f.mac_dst}</span></span>
                <span><span class="details-label">Type :</span> ${f.ethertype} (${f.ethertype_name})</span>
                <span><span class="details-label">Longueur :</span> ${frameLen} octets</span>
              </div>
              <div class="details-item">
                <div class="details-title">IP</div>
                ${ipBlock}
              </div>
              <div class="details-item">
                <div class="details-title">Transport (L4)</div>
                ${l4Block}
              </div>
              ${payloadBlock ? `<div class="details-item" style="grid-column: 1/-1;"><div class="details-title">Aperçu Payload</div>${payloadBlock}</div>` : ""}
            </div>
          </td>
        `;

        tbody.appendChild(tr);
        tbody.appendChild(trDetails);
    }

    document.getElementById("frame-count").innerText = frames.length.toString();
}

function fetchFrames() {
    fetch("/data")
      .then(resp => resp.json())
      .then(data => renderFrames(data))
      .catch(err => console.error("Erreur fetch /data:", err));
}

window.addEventListener("load", () => {
    fetchFrames();
    setInterval(fetchFrames, 2000);
});
</script>
</head>
<body>
<header>
  <h1>Inspecteur Ethernet 10BASE-T / 100BASE-TX</h1>
  <div class="header-controls">
    <span class="frame-count"><span id="frame-count">0</span> trames</span>
    <button class="clear-btn" onclick="clearFrames()">Effacer tout</button>
  </div>
</header>
<main>
<table>
<thead>
<tr>
  <th class="col-id">#</th>
  <th class="col-time">Temps</th>
  <th>MAC source</th>
  <th>MAC destination</th>
  <th class="col-len">Taille</th>
  <th class="col-proto">Protocole</th>
  <th>Info</th>
</tr>
</thead>
<tbody id="frames-body">
</tbody>
</table>
<footer>Cliquer sur une ligne pour afficher/masquer les détails. Mise à jour automatique toutes les 2 secondes.</footer>
</main>
</body>
</html>
"""

@app.route("/")
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route("/data")
def data():
    with lock:
        return jsonify(list(frames))

@app.route("/clear", methods=["POST"])
def clear():
    with lock:
        frames.clear()
    return jsonify({"success": True})

if __name__ == "__main__":
    t = threading.Thread(target=zmq_receiver, daemon=True)
    t.start()
    print("Flask server on http://127.0.0.1:5000/")
    app.run(host="127.0.0.1", port=5000, debug=False)

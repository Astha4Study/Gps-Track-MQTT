let map, marker, mqttClient; // Variabel global untuk peta, marker, dan koneksi MQTT

// Inisialisasi peta menggunakan Leaflet.js
function initMap() {
    // Membuat peta dengan posisi awal (latitude, longitude) dan zoom level 12
    map = L.map('map').setView([-7.431391, 109.247833], 12);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; OpenStreetMap contributors'
    }).addTo(map);

    // Menambahkan marker awal ke peta di posisi yang sama
    marker = L.marker([-7.431391, 109.247833]).addTo(map);
}

// Memperbarui posisi marker dan memindahkan fokus peta
function updatePosition(lat, lng) {
    // Memperbarui posisi marker ke koordinat baru
    marker.setLatLng([lat, lng]);

    // Memindahkan peta ke posisi baru dengan animasi
    map.flyTo([lat, lng], map.getZoom(), { animate: true, duration: 1.5 });
}

// Koneksi ke broker MQTT menggunakan WebSocket
function connectMQTT() {
    const mqttStatus = document.getElementById('mqttStatus'); // Elemen status koneksi MQTT

    // Membuat koneksi ke broker MQTT dengan autentikasi username dan password
    mqttClient = mqtt.connect('wss://yourMqttWebsocket/mqtt', {
        username: 'userNameConnection',
        password: 'passwordConnection',
    });

    // Event handler ketika koneksi berhasil
    mqttClient.on('connect', () => {
        console.log('Terhubung ke MQTT broker');
        mqttStatus.textContent = 'Connected'; // Perbarui teks status
        mqttStatus.classList.remove('bg-red-600'); // Hapus kelas status offline
        mqttStatus.classList.add('bg-green-600', 'text-gray-900'); // Tambahkan kelas status online

        // Subscribe ke topik GPS untuk menerima data posisi
        mqttClient.subscribe('gps', (err) => {
            if (err) {
                console.error('Gagal subscribe ke topik:', err); // Log jika gagal
            } else {
                console.log('Berhasil subscribe ke topik: gps'); // Log jika sukses
            }
        });
    });

    // Event handler untuk menerima pesan dari broker
    mqttClient.on('message', (topic, message) => {
        console.log(`Pesan diterima dari topik "${topic}": ${message}`); // Log pesan ke konsol

        // Proses data hanya jika topiknya adalah 'gps'
        if (topic === 'gps') {
            try {
                const data = JSON.parse(message); // Parsing data JSON dari pesan
                if (data.latitude && data.longitude) {
                    updatePosition(data.latitude, data.longitude); // Perbarui posisi marker
                } else {
                    console.error('Data GPS tidak valid:', data); // Log jika data tidak valid
                }
            } catch (error) {
                console.error('Gagal mem-parsing pesan:', error); // Log jika parsing JSON gagal
            }
        }
    });
}

// Inisialisasi peta dan koneksi MQTT saat halaman dimuat
window.onload = () => {
    initMap(); // Inisialisasi peta
    connectMQTT(); // Koneksi ke broker MQTT
};
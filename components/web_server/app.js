async function refresh() {
    try {
        const res = await fetch('/api/state');
        const data = await res.json();

        document.getElementById('t').innerText = data.target.toFixed(2);
        document.getElementById('a').innerText = data.actual.toFixed(2);
        document.getElementById('h').innerText = data.heater;

        const rt = await fetch('/api/time');
        const dt = await rt.json();

        document.getElementById('time').innerText = dt.time;
    } catch (e) {
        console.error(e);
    }
}

async function up() {
    await fetch('/api/target/up', { method: 'POST' });
    refresh();
}

async function down() {
    await fetch('/api/target/down', { method: 'POST' });
    refresh();
}

async function loadLogs() {
    try {
        const res = await fetch('/logs.csv');
        const text = await res.text();
        const tbody = document.getElementById('logs');
        const rows = text.trim().split('\n');
        tbody.innerHTML = '';
        rows.reverse().forEach(row => {
            const tr = document.createElement('tr');
            row.split(',').forEach(c => {
                const td = document.createElement('td');
                td.innerText = c;
                tr.appendChild(td);
            });
            tbody.appendChild(tr);
        });
    } catch (e) {
        console.error(e);
    }
}

window.onload = () => {
    refresh();
    loadLogs();

    setInterval(refresh, 1000);
    setInterval(loadLogs, 20000);
};

// Reads the local 1mb.json once and reuses the payload for all clients.

const fs = require("fs");
const path = require("path");

const TARGETS = [
  "http://127.0.0.1:8080/stress",
  "http://127.0.0.1:8080/stress2",
  "http://127.0.0.1:8080/",
  "http://127.0.0.1:8080/stress/3",
  "http://127.0.0.1:8080/stress/4",
  "http://127.0.0.1:8080/stress/xxx/4",
  "http://127.0.0.1:8080/stress/x/4",
  "http://127.0.0.1:8080/stress/../4.css",
  "http://127.0.0.1:8080/style.css",
  "http://127.0.0.1:8080/app.js",
  "http://127.0.0.1:8080/../app.js",
];

const CLIENTS = 3000;
const TIMEOUT_MS = 90000; // increased timeout for large uploads

// const payloadPath = path.join(__dirname, "5mb.json");
// let payload;
// try {
//   payload = fs.readFileSync(payloadPath, "utf8");
//   console.log(
//     `Loaded payload from ${payloadPath} (${Buffer.byteLength(
//       payload,
//       "utf8"
//     )} bytes)`
//   );
// } catch (e) {
//   console.error("Failed to read 1mb.json:", e);
//   process.exit(1);
// }
function GetRandomIndex() {
  return Math.floor(Math.random() * TARGETS.length);
}
let count_failed = 0;
async function makeClient(id) {
  try {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), TIMEOUT_MS);

    const res = await fetch(TARGETS[GetRandomIndex()], {
      method: "GET",
      headers: {},
      signal: controller.signal,
    });

    clearTimeout(timeout);

    const text = await res.text().catch(() => "");
    console.log(
      `client ${id} -> ${res.status} (${res.statusText}) ${text
        .slice(0, 120)
        .replace(/\n/g, " ")}`
    );
  } catch (err) {
    count_failed++;
    console.error(
      `client ${id} error: ${err && err.message ? err.message : String(err)}`
    );
  }
}

async function main() {
  const promises = [];
  for (let i = 0; i < CLIENTS; ++i) {
    promises.push(makeClient(i));
  }

  await Promise.all(promises);
  console.log("All clients finished");
  console.log(`Total failed requests: ${count_failed}`);
}

main().catch((err) => console.error("Fatal error:", err));

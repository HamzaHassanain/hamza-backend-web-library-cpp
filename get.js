// Reads the local 1mb.json once and reuses the payload for all clients.

const fs = require("fs");
const path = require("path");

const TARGET = "http://127.0.0.1:8080/stress";
const CLIENTS = 1000;
const TIMEOUT_MS = 60000; // increased timeout for large uploads

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

let count_failed = 0;
async function makeClient(id) {
  try {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), TIMEOUT_MS);

    const res = await fetch(TARGET, {
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
}

main().catch((err) => console.error("Fatal error:", err));

setTimeout(() => {
  console.log(`Total failed requests: ${count_failed}`);
}, 10000);

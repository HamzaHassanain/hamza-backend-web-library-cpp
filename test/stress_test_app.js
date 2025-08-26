/**
 * Stress Test for Hamza Web Framework CRUD API
 *
 * This script tests the performance of the CRUD API by sending a large number of
 * concurrent requests and measuring response times and success rates.
 * It also includes security tests with invalid requests and common attack patterns
 * to verify server robustness against malicious inputs.
 *
 * Requirements:
 * - Node.js
 * - npm packages: node-fetch, cli-progress, colors
 *
 * Install dependencies:
 * npm install node-fetch@2 cli-progress colors
 *
 * Run the test:
 * node stress_test_app.js
 */

const fetch = require("node-fetch");
const cliProgress = require("cli-progress");
const colors = require("colors");

// Configuration
const CONFIG = {
  baseUrl: "http://localhost:3000",
  numItems: 20, // Number of items to create
  concurrentRequests: 50, // Number of concurrent requests per test
  iterations: 5, // Number of iterations for each test
  delayBetweenTests: 1000, // Delay between tests in ms
  timeout: 5000, // Request timeout in ms
  securityTestPercent: 30, // Percentage of requests that should be security/invalid tests
  enableSecurityTests: true, // Whether to include security tests
};

// Test statistics
const stats = {
  create: {
    success: 0,
    fail: 0,
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
  read: {
    success: 0,
    fail: 0,
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
  readAll: {
    success: 0,
    fail: 0,
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
  update: {
    success: 0,
    fail: 0,
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
  delete: {
    success: 0,
    fail: 0,
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
  security: {
    success: 0, // For security tests, success means the server properly rejected the request
    fail: 0, // For security tests, fail means the server improperly accepted the request
    totalTime: 0,
    maxTime: 0,
    minTime: Number.MAX_SAFE_INTEGER,
  },
};

// Generate a random item
function generateItem(index) {
  return {
    name: `Test Item ${index}`,
    description: `This is a test item created for stress testing - ${Math.random()
      .toString(36)
      .substring(2, 15)}`,
    price: parseFloat((Math.random() * 100).toFixed(2)),
  };
}

// Perform a single request with timing
async function timedRequest(url, options, testType) {
  const startTime = Date.now();

  try {
    const response = await Promise.race([
      fetch(url, options),
      new Promise((_, reject) =>
        setTimeout(() => reject(new Error("Request timeout")), CONFIG.timeout)
      ),
    ]);

    const endTime = Date.now();
    const elapsed = endTime - startTime;

    // For security tests, a failure (4xx/5xx) is actually a success
    if (testType === "security") {
      if (!response.ok) {
        // The server correctly rejected the malicious request
        stats[testType].success++;
        stats[testType].totalTime += elapsed;
        stats[testType].maxTime = Math.max(stats[testType].maxTime, elapsed);
        stats[testType].minTime = Math.min(stats[testType].minTime, elapsed);
        return { securityTestSuccess: true, status: response.status };
      } else {
        // The server incorrectly accepted the malicious request

        stats[testType].fail++;
        console.error(
          "\n",
          colors.red(
            `Security test failed! Server accepted malicious request to ${url}\n`
          )
        );
        return { securityTestFailed: true };
      }
    }

    // Normal request handling (non-security tests)
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    // Update statistics
    stats[testType].success++;
    stats[testType].totalTime += elapsed;
    stats[testType].maxTime = Math.max(stats[testType].maxTime, elapsed);
    stats[testType].minTime = Math.min(stats[testType].minTime, elapsed);

    // Special handling for 204 No Content responses - don't try to parse JSON
    if (response.status === 204) {
      return { success: true };
    }

    return await response.json();
  } catch (error) {
    const endTime = Date.now();
    const elapsed = endTime - startTime;

    stats[testType].fail++;
    console.error(`Error in ${testType} request: ${error.message} \n`, "\n");
    return null;
  }
}

// Create items
async function createItems(count) {
  console.log(`\n${"=".repeat(50)}`);
  console.log(`Creating ${count} items...`);

  const progressBar = new cliProgress.SingleBar({
    format:
      "Creating Items |" +
      colors.cyan("{bar}") +
      "| {percentage}% || {value}/{total} items",
    barCompleteChar: "\u2588",
    barIncompleteChar: "\u2591",
    hideCursor: true,
  });

  progressBar.start(count, 0);

  const createdItems = [];

  for (let i = 0; i < count; i++) {
    const item = generateItem(i);
    const response = await timedRequest(
      `${CONFIG.baseUrl}/api/items`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(item),
      },
      "create"
    );

    if (response) {
      createdItems.push(response);
    }

    progressBar.update(i + 1);
  }

  progressBar.stop();
  console.log(`Created ${createdItems.length} items successfully.`);

  return createdItems;
}

// Run multiple concurrent requests for a specific test type
async function runConcurrentRequests(testType, items, requestFn) {
  console.log(`\n${"=".repeat(50)}`);
  console.log(
    `Running ${testType} test with ${CONFIG.concurrentRequests} concurrent requests x ${CONFIG.iterations} iterations`
  );

  const progressBar = new cliProgress.SingleBar({
    format:
      `${testType} Test |` +
      colors.cyan("{bar}") +
      "| {percentage}% || {value}/{total} requests",
    barCompleteChar: "\u2588",
    barIncompleteChar: "\u2591",
    hideCursor: true,
  });

  const totalRequests = CONFIG.concurrentRequests * CONFIG.iterations;
  progressBar.start(totalRequests, 0);

  let completed = 0;

  for (let i = 0; i < CONFIG.iterations; i++) {
    const requests = Array.from({ length: CONFIG.concurrentRequests }, () => {
      const promise = requestFn(items).then(() => {
        progressBar.update(++completed);
      });
      return promise;
    });

    await Promise.all(requests);

    if (i < CONFIG.iterations - 1) {
      await new Promise((resolve) =>
        setTimeout(resolve, CONFIG.delayBetweenTests)
      );
    }
  }

  progressBar.stop();
}

// Read single item test
async function readItemTest(items) {
  const randomIndex = Math.floor(Math.random() * items.length);
  const item = items[randomIndex];

  if (!item) return;

  return await timedRequest(
    `${CONFIG.baseUrl}/api/items/${item.id}`,
    {
      method: "GET",
    },
    "read"
  );
}

// Read all items test
async function readAllItemsTest() {
  return await timedRequest(
    `${CONFIG.baseUrl}/api/items`,
    {
      method: "GET",
    },
    "readAll"
  );
}

// Update item test
async function updateItemTest(items) {
  const randomIndex = Math.floor(Math.random() * items.length);
  const item = items[randomIndex];

  if (!item) return;

  const updatedItem = {
    name: `Updated ${item.name}`,
    description: `Updated description - ${Math.random()
      .toString(36)
      .substring(2, 15)}`,
    price: parseFloat((Math.random() * 200).toFixed(2)),
  };

  return await timedRequest(
    `${CONFIG.baseUrl}/api/items/${item.id}`,
    {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(updatedItem),
    },
    "update"
  );
}

// Delete item test
async function deleteItemTest(items) {
  // Remove an item from the array and return it
  const randomIndex = Math.floor(Math.random() * items.length);
  const item = items.splice(randomIndex, 1)[0];

  if (!item) return;

  return await timedRequest(
    `${CONFIG.baseUrl}/api/items/${item.id}`,
    {
      method: "DELETE",
    },
    "delete"
  );
}

// Security Tests
// Generate a random security test
async function securityTest(items) {
  // List of security test types
  const testTypes = [
    "sqlInjection",
    "xss",
    "invalidJson",
    "oversizedPayload",
    "malformedId",
    "methodNotAllowed",
    "invalidContentType",
    "nonExistentEndpoint",
  ];

  // Randomly select a test type
  const testType = testTypes[Math.floor(Math.random() * testTypes.length)];

  // If items array is empty or undefined, use a fallback ID
  const fallbackId = "123456789";
  const randomItem =
    items && items.length > 0
      ? items[Math.floor(Math.random() * items.length)]
      : { id: fallbackId };

  switch (testType) {
    case "sqlInjection":
      // SQL Injection tests
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items/${randomItem.id}' OR '1'='1`,
        { method: "GET" },
        "security"
      );

    case "xss":
      // XSS tests
      const xssPayload = {
        name: `<script>alert('XSS')</script>`,
        description: `<img src="x" onerror="alert('XSS')">`,
        price: 10.99,
      };
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(xssPayload),
        },
        "security"
      );

    case "invalidJson":
      // Invalid JSON format
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: `{"name": "Invalid JSON, "description": "Missing quote", price: 10.99}`,
        },
        "security"
      );

    case "oversizedPayload":
      // Oversized payload
      const hugeDescription = "x".repeat(1024 * 1000);
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            name: "Huge Item",
            description: hugeDescription,
            price: 99.99,
          }),
        },
        "security"
      );

    case "malformedId":
      // Malformed ID parameter
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items/null`,
        { method: "GET" },
        "security"
      );

    case "methodNotAllowed":
      // Method not allowed
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items`,
        { method: "PATCH" },
        "security"
      );

    case "invalidContentType":
      // Invalid content type
      return await timedRequest(
        `${CONFIG.baseUrl}/api/items`,
        {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          body: "This is not JSON",
        },
        "security"
      );

    case "nonExistentEndpoint":
      // Non-existent endpoint
      return await timedRequest(
        `${CONFIG.baseUrl}/api/nonexistent/${Math.random()
          .toString(36)
          .substring(7)}`,
        { method: "GET" },
        "security"
      );

    default:
      return null;
  }
}

// Print test results
function printResults() {
  console.log(`\n${"=".repeat(50)}`);
  console.log(colors.bold.underline("STRESS TEST RESULTS"));
  console.log("=".repeat(50));

  const printStats = (type) => {
    const s = stats[type];
    const total = s.success + s.fail;
    const successRate = total > 0 ? ((s.success / total) * 100).toFixed(2) : 0;
    const avgTime = s.success > 0 ? (s.totalTime / s.success).toFixed(2) : 0;

    console.log(colors.bold(`\n${type.toUpperCase()} OPERATIONS:`));
    console.log(`Total Requests: ${total}`);
    console.log(`Success: ${colors.green(s.success)} (${successRate}%)`);
    console.log(`Failed: ${colors.red(s.fail)}`);
    console.log(`Average Response Time: ${avgTime} ms`);
    if (s.success > 0) {
      console.log(`Min Response Time: ${s.minTime} ms`);
      console.log(`Max Response Time: ${s.maxTime} ms`);
    }
  };

  printStats("create");
  printStats("read");
  printStats("readAll");
  printStats("update");
  printStats("delete");

  if (CONFIG.enableSecurityTests) {
    printStats("security");
  }

  const totalSuccess = Object.values(stats).reduce(
    (sum, s) => sum + s.success,
    0
  );
  const totalFail = Object.values(stats).reduce((sum, s) => sum + s.fail, 0);
  const totalRequests = totalSuccess + totalFail;
  const overallSuccessRate = ((totalSuccess / totalRequests) * 100).toFixed(2);

  console.log(`\n${"=".repeat(50)}`);
  console.log(colors.bold.underline("OVERALL SUMMARY:"));
  console.log(`Total Requests: ${totalRequests}`);
  console.log(`Overall Success Rate: ${colors.green(overallSuccessRate)}%`);
  console.log("=".repeat(50));
}

// Main function
async function main() {
  console.log(
    colors.bold.yellow(`
    =========================================
    HAMZA WEB FRAMEWORK - API STRESS TESTER
    =========================================
    `)
  );

  console.log(
    colors.cyan(`Configuration:
    - Base URL: ${CONFIG.baseUrl}
    - Items to create: ${CONFIG.numItems}
    - Concurrent requests: ${CONFIG.concurrentRequests}
    - Iterations: ${CONFIG.iterations}
    - Timeout: ${CONFIG.timeout}ms
    - Security tests: ${CONFIG.enableSecurityTests ? "Enabled" : "Disabled"}
    `)
  );

  try {
    // Create initial test items
    const items = await createItems(CONFIG.numItems);

    if (items.length === 0) {
      console.error(colors.red("Failed to create test items. Aborting test."));
      return;
    }

    // Make a copy of items array that can be modified during the delete test
    let itemsCopy = [...items];

    // Run read single item test
    await runConcurrentRequests("Read Single Item", items, readItemTest);

    // Run read all items test
    await runConcurrentRequests("Read All Items", items, readAllItemsTest);

    // Run update items test
    await runConcurrentRequests("Update Item", items, updateItemTest);

    // Run delete items test (using the copy that will be modified)
    await runConcurrentRequests("Delete Item", itemsCopy, deleteItemTest);

    // Run security tests if enabled
    if (CONFIG.enableSecurityTests) {
      await runConcurrentRequests("Security Testing", items, securityTest);
    }

    // Print results
    printResults();

    // Clean up any remaining items
    console.log("\nCleaning up remaining items...");
    for (const item of items) {
      await fetch(`${CONFIG.baseUrl}/api/items/${item.id}`, {
        method: "DELETE",
      }).catch((e) =>
        console.error(`Failed to clean up item ${item.id}: ${e.message}`)
      );
    }
  } catch (error) {
    console.error(colors.red(`\nFailed to run stress test: ${error.message}`));
  }
}

// Run the test
main().catch((error) => {
  console.error(colors.red(`\nUnhandled error: ${error.message}`));
  console.error(error.stack);
});

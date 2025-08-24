# Hamza Web Framework Stress Test

This directory contains a stress test script for the Hamza Web Framework CRUD API. The script helps evaluate how the C++ web framework handles concurrent requests and load.

## Prerequisites

- Node.js (v12 or higher)
- npm

## Installation

Install the required dependencies:

```bash
npm install node-fetch@2 cli-progress colors
```

Note: We're using node-fetch v2 because it provides a simpler CommonJS import (v3+ requires ES modules).

## Running the Stress Test

Make sure your C++ web framework server is running on `http://localhost:3000` (or update the `baseUrl` in the script if it's running on a different address).

Then run the stress test:

```bash
node stress_test_app.js
```

## Configuration

You can modify the following parameters in the `CONFIG` object at the top of the script:

- `baseUrl`: The base URL of your API server
- `numItems`: Number of items to create for testing
- `concurrentRequests`: Number of concurrent requests per test
- `iterations`: Number of iterations for each test
- `delayBetweenTests`: Delay between test iterations in milliseconds
- `timeout`: Request timeout in milliseconds

## Test Workflow

The stress test performs the following operations:

1. Creates test items
2. Tests reading single items with concurrent requests
3. Tests reading all items with concurrent requests
4. Tests updating items with concurrent requests
5. Tests deleting items with concurrent requests
6. Displays detailed statistics for each operation
7. Cleans up any remaining test items

## Statistics

The test reports the following statistics for each operation type:

- Total number of requests
- Success rate
- Failed requests
- Average response time
- Minimum response time
- Maximum response time

## Example Output

```
=========================================
HAMZA WEB FRAMEWORK - API STRESS TESTER
=========================================

Configuration:
    - Base URL: http://localhost:3000
    - Items to create: 20
    - Concurrent requests: 50
    - Iterations: 5
    - Timeout: 5000ms

==================================================
Creating 20 items...
Creating Items |██████████████████████████████████| 100% || 20/20 items
Created 20 items successfully.

==================================================
Running Read Single Item test with 50 concurrent requests x 5 iterations
Read Single Item Test |██████████████████████████████████| 100% || 250/250 requests

==================================================
Running Read All Items test with 50 concurrent requests x 5 iterations
Read All Items Test |██████████████████████████████████| 100% || 250/250 requests

==================================================
Running Update Item test with 50 concurrent requests x 5 iterations
Update Item Test |██████████████████████████████████| 100% || 250/250 requests

==================================================
Running Delete Item test with 50 concurrent requests x 5 iterations
Delete Item Test |██████████████████████████████████| 100% || 250/250 requests

==================================================
STRESS TEST RESULTS
==================================================

CREATE OPERATIONS:
Total Requests: 20
Success: 20 (100.00%)
Failed: 0
Average Response Time: 24.25 ms
Min Response Time: 16 ms
Max Response Time: 38 ms

READ OPERATIONS:
Total Requests: 250
Success: 250 (100.00%)
Failed: 0
Average Response Time: 13.84 ms
Min Response Time: 6 ms
Max Response Time: 48 ms

READALL OPERATIONS:
Total Requests: 250
Success: 250 (100.00%)
Failed: 0
Average Response Time: 16.22 ms
Min Response Time: 7 ms
Max Response Time: 55 ms

UPDATE OPERATIONS:
Total Requests: 250
Success: 250 (100.00%)
Failed: 0
Average Response Time: 19.64 ms
Min Response Time: 8 ms
Max Response Time: 62 ms

DELETE OPERATIONS:
Total Requests: 250
Success: 250 (100.00%)
Failed: 0
Average Response Time: 14.77 ms
Min Response Time: 5 ms
Max Response Time: 51 ms

==================================================
OVERALL SUMMARY:
Total Requests: 1020
Overall Success Rate: 100.00%
==================================================

Cleaning up remaining items...
```

from flask import Flask, request, jsonify
from flask_cors import CORS
import os

app = Flask(__name__)
CORS(app)

latest_data = {
    "flow1": 0,
    "flow2": 0,
    "loss": 0,
    "flowLossPercent": 0,
    "sourceDistance": 0,
    "outletDistance": 0,
    "sourceDrop": 0,
    "outletRise": 0,
    "levelMismatch": 0,
    "turbidity": 0,
    "waterType": "Unknown",
    "qualityLevel": "Unknown",
    "status": "Normal",
    "recommendation": "Readings are within the 10-unit margin. Continue normal monitoring."
}

@app.route("/")
def home():
    return "Aqua-Shield backend is running"

@app.route("/update", methods=["POST"])
def update():
    global latest_data
    data = request.get_json()

    if not data:
        return jsonify({"error": "No data received"}), 400

    print("Received:", data)
    latest_data = data

    return jsonify({
        "message": "Data updated",
        "data": latest_data
    })

@app.route("/data", methods=["GET"])
def get_data():
    return jsonify(latest_data)

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port)
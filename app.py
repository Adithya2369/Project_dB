from flask import Flask, request, jsonify

app = Flask(__name__)

# Dictionary containing allowed dB levels for different locations
allowed_db_levels = {
    "newyork": 70,
    "losangeles": 65,
    "chicago": 68,
    "houston": 72,
    "sanfrancisco": 64,
    "boston": 66
}

@app.route('/info', methods=['GET'])
def get_allowed_db():
    location = request.args.get('location', '').strip().lower()  # Convert location to lowercase
    allowed_db = allowed_db_levels.get(location, "Location not found")  # Fetch dB level
    return jsonify({"allowed_dB": allowed_db})

if __name__ == '__main__':
    app.run(debug=True)

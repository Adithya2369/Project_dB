from flask import Flask, request, jsonify
import json

app = Flask(__name__)

# Load allowed dB levels from an external JSON file
def load_db_levels():
    try:
        with open('allowed_db_levels.json', 'r') as file:
            return json.load(file)
    except FileNotFoundError:
        return {}

@app.route('/info', methods=['GET'])
def get_allowed_db():
    allowed_db_levels = load_db_levels()  # Load the dictionary from the JSON file
    location = request.args.get('location', '').strip().lower()  # Convert location to lowercase
    allowed_db = allowed_db_levels.get(location, 150)  # Fetch dB level
    return jsonify({"allowed_dB": allowed_db})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)

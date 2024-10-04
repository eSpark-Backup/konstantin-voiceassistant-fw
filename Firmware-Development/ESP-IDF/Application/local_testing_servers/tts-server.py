import os
import pyttsx3
import threading
from flask import Flask, request, send_file, jsonify
from io import BytesIO
from pydub import AudioSegment
import tempfile
import imageio_ffmpeg
import subprocess

# Set the path to imageio's ffmpeg
AudioSegment.converter = imageio_ffmpeg.get_ffmpeg_exe()

# Define a specific temporary directory
TEMP_DIR = os.path.join(os.getcwd(), "temp_files")

# Ensure the directory exists
os.makedirs(TEMP_DIR, exist_ok=True)

app = Flask(__name__)

# Initialize pyttsx3 engine
engine = pyttsx3.init()

# Set speech properties (optional)
engine.setProperty('rate', 150)  # Speed of speech
engine.setProperty('volume', 1)  # Volume level (0.0 to 1.0)

voices = engine.getProperty('voices')

def run_tts(text, wav_file_path):
    """Run the pyttsx3 engine and save output to a WAV file."""
    engine.save_to_file(text, wav_file_path)
    engine.runAndWait()

def play_audio(wav_file_path):
    """Play audio from a WAV file using ffplay."""
    subprocess.Popen(["ffplay", "-nodisp", "-autoexit", wav_file_path])

@app.route('/convert', methods=['POST'])
def convert_text_to_speech():
    # Print request headers
    print("Request Headers:")
    print(dict(request.headers))

    # Print request body
    print("Request Body:")
    print(request.get_data(as_text=True))

    # Get the text from the POST request
    data = request.get_json()
    if 'input' not in data:
        return jsonify({'error': 'No text provided'}), 400

    text = data['input']

    # Use a temporary file to store the generated WAV file
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav", dir=TEMP_DIR) as wav_file:
        wav_file_path = wav_file.name

    # Run TTS in a separate thread to avoid blocking
    tts_thread = threading.Thread(target=run_tts, args=(text, wav_file_path))
    tts_thread.start()
    tts_thread.join()  # Wait for TTS process to finish

    # Play the generated WAV file on the server
    # play_audio(wav_file_path)

    # Convert WAV to MP3 using pydub
    audio_segment = AudioSegment.from_wav(wav_file_path)
    mp3_buffer = BytesIO()
    audio_segment.export(mp3_buffer, format="mp3")

    # Seek to the beginning of the MP3 buffer
    mp3_buffer.seek(0)

    # Remove the temporary WAV file manually
    os.remove(wav_file_path)

    # Log response before sending
    response_info = {
        'response_type': 'file',
        'file_name': 'speech.mp3',
        'mimetype': 'audio/mpeg'
    }
    print("Response Info:", response_info)

    # Send the MP3 file as a response
    return send_file(
        mp3_buffer, 
        mimetype='audio/mpeg',
        as_attachment=True,
        download_name='speech.mp3'
    )

if __name__ == "__main__":
    # Run the Flask app
    app.run(debug=True, host='0.0.0.0', port=5000, threaded=True)

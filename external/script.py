import io
import os
import argparse
import subprocess
from google.cloud import speech
from google.cloud.speech import enums
from google.cloud.speech import types


parser = argparse.ArgumentParser(
    description='Ogg voice to text transcriptor')

parser.add_argument('--input', help='path to ogg voice message')

args = vars(parser.parse_args())

path_to_input_ogg = args['input']

#os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = path_to_config

encoding = enums.RecognitionConfig.AudioEncoding.LINEAR16
dest_WAV_filename = path_to_input_ogg + '.wav'
process = subprocess.run(['ffmpeg', '-i', path_to_input_ogg, dest_WAV_filename])

client = speech.SpeechClient()

# Loads the audio into memory
with io.open(dest_WAV_filename, 'rb') as audio_file:
    content = audio_file.read()
    audio = types.RecognitionAudio(content=content)
    
os.remove(dest_WAV_filename)

config = types.RecognitionConfig(
    encoding=encoding,
    sample_rate_hertz=48000,
    language_code='ru-RU')

response = client.recognize(config, audio)

for result in response.results:
    print(result.alternatives[0].transcript)

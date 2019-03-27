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

parser.add_argument('--config', help='path to your google cloud config')

parser.add_argument('--opus', help='is your file in opus format')

args = vars(parser.parse_args())

path_to_config = args['config']
path_to_input_ogg = args['input']
is_opus = args['opus'] == 'T'
encoding = None

os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = path_to_config

if not is_opus:
    # Save OGG to one channel WAV
    encoding = enums.RecognitionConfig.AudioEncoding.LINEAR16
    dest_WAV_filename = path_to_input_ogg + '.wav'
    process = subprocess.run(['ffmpeg', '-i', path_to_input_ogg, dest_WAV_filename])
else:
    encoding = enums.RecognitionConfig.AudioEncoding.OGG_OPUS
    dest_WAV_filename = path_to_input_ogg

client = speech.SpeechClient()

# Loads the audio into memory
with io.open(dest_WAV_filename, 'rb') as audio_file:
    content = audio_file.read()
    audio = types.RecognitionAudio(content=content)

config = types.RecognitionConfig(
    encoding=encoding,
    sample_rate_hertz=48000,
    language_code='ru-RU')

response = client.recognize(config, audio)

for result in response.results:
    print('Transcript: {}'.format(result.alternatives[0].transcript))


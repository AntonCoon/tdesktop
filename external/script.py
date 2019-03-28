import requests
import io
import base64
import json
import argparse
import subprocess
import os


parser = argparse.ArgumentParser(
    description='Ogg voice to text transcriptor')
parser.add_argument('--input', help='path to ogg voice message')

args = vars(parser.parse_args())

path_to_input_ogg = args['input']
#os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = path_to_config
dest_WAV_filename = path_to_input_ogg + '.wav'
process = subprocess.run(['ffmpeg', '-i', path_to_input_ogg, dest_WAV_filename])

# Loads the audio into memory
with io.open(dest_WAV_filename, 'rb') as audio_file:
    content = audio_file.read()
    audio = base64.b64encode(content).decode()
    
os.remove(dest_WAV_filename)

headers = {
    "Content-Type": "application/json; charset=utf-8", 
}

params = {
  "config": {
    'languageCode': 'ru',
  },
  "audio": {
    'content': audio
  }
}
api_key = os.environ["GOOGLE_APPLICATION_API_KEY"]
response = requests.post(
        'https://speech.googleapis.com/v1/speech:recognize?key=%s' % api_key,
        data=json.dumps(params),
        headers=headers,
        timeout=5
    )
res = response.json()
if 'results' in res:
    print(res['results'][0]['alternatives'][0]['transcript'])
else:
    print('Failure')
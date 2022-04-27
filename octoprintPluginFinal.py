#!/usr/bin/env python3

import requests
import sys
import time
import datetime
import json
import serial

OCTOPRINT_API_KEY = '2B08193846044C8B987A19AF1C258DCF'
OCTOPRINT_HOST = 'localhost'
OCTOPRINT_PORT = '80'
EJECTOR_TTY = '/dev/ttyACM1'
COOLING_TIME = 30 # 30 seconds
AUTO_RESTART = False

# Initialize Printer status
STATUS_IDLE = 0
STATUS_PRINTING = 1
STATUS_COOLING = 2
STATUS_EJECTING = 3
STATUS_RESTARTING = 4


def log(text):
    print(datetime.datetime.now().strftime('%b %d %H:%M:%S ') + text)

log('Initalizing')

# Init OctoPrint connection
Session = requests.Session()
Session.headers.update({
    'X-Api-Key': OCTOPRINT_API_KEY,
    'Content-Type': 'application/json'
})
BaseAdress = 'http://' + OCTOPRINT_HOST + ':' + str(OCTOPRINT_PORT)

# Init Arduino connection
Serial = serial.Serial(EJECTOR_TTY, 115200, timeout=1) # 1s timeout

# Let time to initialize
time.sleep(10)

# Test ejector connection
Serial.write(b"PING\n")
if Serial.readline().decode('utf-8').rstrip() == 'PONG':
    log("Ejector connected")
else:
    raise Exception('Error connecting to ejector')


# Test octoprint connection
Result = Session.get(BaseAdress + '/api/printer')
if Result.status_code != 200:
    raise Exception('Error trying to get printer connection status')
try:
    Status = json.loads(Result.content.decode('utf-8'))["state"]["flags"]["operational"]  
    if Status is True:
        log('Printer connected, starting monitoring')
    else:
        raise Exception('Error trying to get printer connection status')
except:
    raise Exception('Error trying to get printer connection status')

# Function for commands to extract information from printer (get print status, etc.)
def getUrl(path):
    while True:
        try:
            Result = Session.get(BaseAdress + path)

            if Result.status_code != 200:
                raise Exception("Error: {code} - {content}".format(code=Result.status_code, content=Result.content.decode('utf-8')))

            return json.loads(Result.content.decode('utf-8'))
        except Exception as exc:
            log(str(exc))
            time.sleep(1)

# Function for command to send to printer (start/pause print, etc.)
def postUrl(path, data):
    while True:
        try:
            result = Session.post(BaseAdress + path, json=data)
            if result.status_code != 204:
                raise Exception("Error: {code} - {content}".format(code=result.status_code, content=result.content.decode('utf-8')))

            return result
        except Exception as exc:
            log(str(exc))
            time.sleep(1)


# Initialize variables for ejection loop
printerStatus = STATUS_IDLE
coolingStart = 0
sensorError = True
errorCount = 0

while True:
	# Check for octoprint status:
	Status = getUrl('/api/printer')["state"]["text"]

	# If printer is currently in progress. Might need to change 'Printing from SD' to 'Printing' based on how you are uploading gcode files
	if Status == 'Printing from SD':
		if printerStatus is not STATUS_PRINTING:
			log("Printer is printing")
			printerStatus = STATUS_PRINTING

	# If print has just been completed
	elif Status == 'Operational':
		if printerStatus is STATUS_PRINTING:
			log("Print finsihed, cooling for " + str(COOLING_TIME/60) + "min...")
			coolingStart = time.time()
			printerStatus = STATUS_COOLING
		# Cool down for set amount of time in COOLING_TIME at top of code
		elif printerStatus is STATUS_COOLING:
			if time.time() - coolingStart >= COOLING_TIME:
				log("Cooling Finished. Preparing to Eject")
				printerStatus = STATUS_EJECTING
		# Begin ejection process when cooling complete
		elif printerStatus is STATUS_EJECTING:
			while sensorError == True:
				# Send EJECT serially to Arduino
				Serial.write(b"EJECT\n")
				Result = Serial.readline().decode('utf-8').rstrip()
				if Result == 'START':
                			log("Ejection start")
				else:
					log('Wrong result:' + Result)
					raise Exception('Error trying to start ejection')

				# Wait for ejection to end. Arm takes around 90 seconds to fully eject device. 
				# If sleep is not here, execution ends due to wrong serial result from Arduino (thinks there is an error)
				time.sleep(100)
				Result = Serial.readline().decode('utf-8').rstrip()
				if Result == 'END':
					sensorError = False
					log("Ejection done")
				# If ERROR is received, repeat ejection process up to 3 times before stopping execution
				elif Result == 'ERROR':
					sensorError = True
					errorCount += 1
					if errorCount == 3:
						log("3 unsuccessful attempts to remove print. Stopping Execution.")
						quit()
				else:
					log('Wrong result:' + Result)
					raise Exception('Error waiting for ejection end')

			# If AUTO_RESTART is set to True, device will start next print in queue. If False, the program will end
			if AUTO_RESTART:
				printerStatus = STATUS_RESTARTING
			else:
				printerStatus = STATUS_IDLE

		# Send command to printer to start next print
		elif printerStatus is STATUS_RESTARTING:
			postUrl('/api/job', {'command': 'start'})
			log("Next print started")
			printerStatus = STATUS_IDLE

	time.sleep(10)


# Temperature commands if interested in using
# Temps = getURL('/api/printer')["temperature"]
# tempBed = getUrl('/api/printer/bed')
# tempNozzle = getUrl('/api/printer/tool')
# print(tempBed)
# print(tempNozzle)
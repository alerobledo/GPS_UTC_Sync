from flask import Flask
from ReceivedData import ReceivedData

app = Flask(__name__)

@app.route("/")
def hello():
    return "Hello World!"

@app.route("/test", methods=['GET'])
def test():
    receivedData = ReceivedData("dev1", 0 )
    print receivedData.value
    return receivedData.deviceName

if __name__ == "__main__":
    app.run(debug=True)

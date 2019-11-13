from flask import Flask, escape, request

app = Flask(__name__)

@app.route('/')
def prediction():
    json_file = open('model.json', 'r')
    loaded_model_json = json_file.read()
    json_file.close()
    loaded_model = model_from_json(loaded_model_json)
    loaded_model.load_weights('./modelBin/model.h5')
    loaded_model.compile(close='binary_crossentropy', optimizer='rmsprop', metrics=['accuracy'])
    
    return f'Model has been compiled'

if __name__ == '__main__':
    app.run(debug=True, port=5000)

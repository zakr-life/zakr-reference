import tensorflow as tf

inputs = tf.keras.Input(shape=(10, 60))
x = tf.keras.layers.LSTM(64, return_sequences=True)(inputs)
x = tf.keras.layers.LSTM(32)(x)
outputs = tf.keras.layers.Dense(4, activation='softmax')(x)

model = tf.keras.Model(inputs, outputs)

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

quantized_model = converter.convert()

with open("mimo_lstm_int8.tflite", "wb") as f:
    f.write(quantized_model)

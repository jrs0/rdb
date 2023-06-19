# Run this script interactively, stepping through the lines 
# one by one and reading the comments. It is just the tutorial
# here: https://www.tensorflow.org/tutorials/quickstart/beginner
# with a bit more information.

# Add path to msys2/mingw dependencies of pybind module. 
import os
os.add_dll_directory("C:/msys64/mingw64/bin")

import pbtest

import tensorflow as tf
from matplotlib import pyplot as plt
import numpy as np

print("TensorFlow version:", tf.__version__)

mnist = tf.keras.datasets.mnist

# The mnist dataset contains handwritten digits. There are
# 60000 digits in the training set and 10000 in the test set.
# Each set has x (the digit in pixel format) and y (what the
# digit actually is). More context is available from here:
# http://yann.lecun.com/exdb/mnist/. The data comes with 
# greyscale one-byte-per-pixel (0-255), so the data are 
# renormalised to values between 0 and 1
(x_train, y_train), (x_test, y_test) = mnist.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0

# x_train and x_test are 3D numpy arrays, where the first
# dimension picks out a digit, and the second two contain the
# pixel data.
type(x_train)
x_train.shape
x_test.shape

# You can plot one of them, for example. The first index
# picks a digit dimension. Change n to pick a random digit
# and plot it.
n = 20
plt.imshow(x_train[n], interpolation='nearest')
plt.title(f"The digit is {y_train[n]}")
plt.show()

# Making a model involves specifying a list of steps
# (layers). The flatten layer turns the 2D 28x28
# array into a vector. The dense layer is a single layer
# of neural network, with 128 nodes (mapping the original
# 784 values down to 128). The activation "relu" maps
# negative values to zero and leaves positive values alone,
# so makes the vector non-negative. The dropout layer randomly
# sets 20% of the 128 values to 0 during training to prevent
# overfitting. The final neural network layer Dense maps the
# result down to a vector or length 10. This 10 corresponds to
# the number of classes (there are 10 digits); the idea is that
# each number is a log-odds for that class (so it can be converted
# into a vector of probabilities for each class)
model = tf.keras.models.Sequential([
  tf.keras.layers.Flatten(input_shape=(28, 28)),
  tf.keras.layers.Dense(128, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(10)
])

# Pick out just the first digit, and run the (untrained)
# model on it. You get 10 numbers, the ones from the final
# layer of the neural network. (Note there are negatives, due
# to the lack of relu in the final Dense layer.) These 10 
# numbers constitute the model prediction.
predictions = model(x_train[:1]).numpy()
predictions

# Turn the vector of 10 log-odds into a vector of probabilities
# (0 to 1 instead of any real number). Note how all these numbers
# are roughly 10%, meaning the model can't distinguish between any
# of the classes.
probs = tf.nn.softmax(predictions).numpy()
probs

# No training has happened yet. The loss function below defines the
# optimisation function for the training. For each probs vector
# like the one above, it picks out the log odds corresponding to the
# right class and takes the log of the probability. This number is always
# negative (since the probs are less than one) and will be more negative
# for smaller numbers (i.e. when the model is getting it wrong more badly).
# This number is negated to get the loss function (higher is worse model
# performance). Since the model is untrained, the probability of the
# correct class is about 10% (random), so -log(0.1) ~ 2.3
loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
loss_fn(y_train[:1], predictions).numpy()

# Compile the model. The optimiser adam is stochastic gradient descent, which
# is trying to minimise the sum of squared loss functions across the test set.
# The metrics just prints additional information evaluated during the training
# and testing.
model.compile(optimizer='adam',
              loss=loss_fn,
              metrics=['accuracy'])

# Fit and evaluate the model
model.fit(x_train, y_train, epochs=5)
model.evaluate(x_test,  y_test, verbose=2)

# Now that the model is fitted, you can repeat the test of the
# models performance. This snippet makes a digit prediction by
# taking the index of the element with the largest class probability.
#
n = 5
true_digit = y_test[n]
log_odds = model(x_test[n:n+1])
probs = tf.nn.softmax(log_odds).numpy()
predicted_digit = np.argmax(probs, axis= -1)
print(f"True digit: {true_digit}; predicted digit: {predicted_digit}")


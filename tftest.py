# Run this script interactively, stepping through the lines 
# one by one and reading the comments. It is just the tutorial
# here: https://www.tensorflow.org/tutorials/quickstart/beginner
# with a bit more information.

# Add path to msys2/mingw dependencies of pybind module. 
import os
os.add_dll_directory("C:/msys64/mingw64/bin")
import pbtest
import pandas as pd
import numpy as np

acs = pbtest.make_acs_dataset("scripts/config.yaml")
df = pd.DataFrame.from_dict(acs)

# Reduce the bleeding column to 1 (for some bleeding)
# or 0 (for no bleeding)
df.bleeding = df.bleeding.apply(lambda x: 1 if x > 0 else 0)

# Make the training and test sets
msk = np.random.rand(len(df)) < 0.75
df_train = df[msk]
df_test = df[~msk]

# Get the outcome column (bleeding)
y_train = df_train.bleeding.to_numpy()
y_test = df_test.bleeding.to_numpy()

# Get the predictors 
x_train = df_train.loc[:, df.columns != "bleeding"].to_numpy()
x_test = df_test.loc[:, df.columns != "bleeding"].to_numpy()

import tensorflow as tf
from matplotlib import pyplot as plt

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
num_hidden_nodes = x_train.shape[1]
model = tf.keras.models.Sequential([
  #tf.keras.layers.Flatten(input_shape=(28, 28)),
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(2)
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
log_odds = model(x_test)
probs = tf.nn.softmax(log_odds).numpy()
prob_of_bleeding = probs[:,1]

import sklearn.metrics as metrics
fpr, tpr, threshold = metrics.roc_curve(y_test, prob_of_bleeding)
roc_auc = metrics.auc(fpr, tpr)
plt.title('Receiver Operating Characteristic')
plt.plot(fpr, tpr, 'b', label = 'AUC = %0.2f' % roc_auc)
plt.legend(loc = 'lower right')
plt.plot([0, 1], [0, 1],'r--')
plt.xlim([0, 1])
plt.ylim([0, 1])
plt.ylabel('True Positive Rate')
plt.xlabel('False Positive Rate')
plt.show()
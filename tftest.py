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
import sklearn.metrics as metrics
import tensorflow as tf
from matplotlib import pyplot as plt

# Load the data
acs = pbtest.make_acs_dataset("scripts/config.yaml")
df = pd.DataFrame.from_dict(acs)
df = df.drop(columns=["age"])

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

# Neural network model
num_hidden_nodes = x_train.shape[1]
model = tf.keras.models.Sequential([
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(2)
])

loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
model.compile(optimizer='adam',
              loss=loss_fn,
              metrics=['accuracy'])

# Fit and evaluate the model
model.fit(x_train, y_train, epochs=5)
model.evaluate(x_test,  y_test, verbose=2)

# Obtain the probabilities of bleeding
log_odds = model(x_test)
probs = tf.nn.softmax(log_odds).numpy()
prob_of_bleeding = probs[:,1]

# Plot ROC curve
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
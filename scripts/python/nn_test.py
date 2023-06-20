# This is a proof-of-principle test for using neural networks
# to analyse the numerical

# Add path to msys2/mingw dependencies of pybind module. 
import os
os.add_dll_directory("C:/msys64/mingw64/bin")
import pbtest
import pandas as pd
import numpy as np
import sklearn.metrics as metrics
import tensorflow as tf
import tensorflow_transform as tft
from matplotlib import pyplot as plt

# Load the data
acs = pbtest.make_acs_dataset("scripts/config.yaml")
df = pd.DataFrame.from_dict(acs)

# Pick out valid rows to keep in the training and test
# data. (Currently, only predict when age is present.)
def valid_rows(df):
    return df.age != -1

df = df[valid_rows(df)]

# Reduce the bleeding column to 1 (for some bleeding)
# or 0 (for no bleeding)
df.bleeding = df.bleeding.apply(lambda x: 1 if x > 0 else 0)

# Make the training and test sets
msk = np.random.rand(len(df)) < 0.75
df_train = df[msk]
df_test = df[~msk]

# Normalise the age column in df using the data 
# in the training set
min_train_age = df_train.age.min()
max_train_age = df_train.age.max()

def min_max_scaler(age):
    return (age - min_train_age) / (max_train_age - min_train_age)

df_train['age'] = df_train['age'].apply(min_max_scaler)
df_test['age'] = df_test['age'].apply(min_max_scaler)

# Get the predictors 
x_train = df_train.drop(columns = "bleeding").to_numpy()
x_test = df_test.drop(columns = "bleeding").to_numpy()

# Get the outcome column (bleeding)
y_train = df_train.bleeding.to_numpy()
y_test = df_test.bleeding.to_numpy()

# Neural network model
num_hidden_nodes = x_train.shape[1]
model = tf.keras.models.Sequential([
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(num_hidden_nodes, activation='relu'),
  tf.keras.layers.Dropout(0.2),
  tf.keras.layers.Dense(2, activation = 'softmax')
])

loss_fn = tf.keras.losses.SparseCategoricalCrossentropy()
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
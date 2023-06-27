# Testing using all diagnosis/procedure codes as the input to a neural
# network to predict bleeding. The outcome column, "bleeding", counts
# the number of times a bleeding code occurred in the episodes up to
# 1 year after the index event. There is one predictor column per
# ICD-10/OPCS-4 code, which contains a count of the number of times this
# code occurred in the episodes up to 1 year before the index event.
#
# NOTE: currently, the predictors include procedure codes found in the
# secondary procedure columns of the index event. These should not be
# included, because they may contain information about the bleeding 
# outcome being predicted. This is just a test script to see roughly
# how well the neural network does -- to be fixed if it looks promising.
#
# 
#

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
acs = pbtest.all_icd_codes("scripts/config.yaml")
raw_df = pd.DataFrame.from_dict(acs)

# Pick out valid rows to keep in the training and test
# data. (Currently, only predict when age is present.)
def valid_rows(df):
    return df.age != -1

# Drop invalid rows, and remove columns
df_all_codes = raw_df[valid_rows(raw_df)]
#df = df.drop(columns=["stemi","age","index_type"])

plt.matshow(df_all_codes.corr())
plt.show()

# Remove the columns which have *really* low numbers of
# non-zero values. Starting with 7000 codes, even setting
# a threshold at 99% only retain 219 columns (i.e. nearly 
# all the codes have less than 1% non-zero values).
all_code_columns = df_all_codes.drop(columns = ["bleeding", "age", "stemi", "pci_medman"])
code_percent_zero = (all_code_columns == 0).sum(axis=0)/(len(df_all_codes))
codes_to_drop = code_percent_zero[code_percent_zero > 0.0].index.to_list()
df = df_all_codes.drop(columns = codes_to_drop)

# Reduce the bleeding column to 1 (for some bleeding)
# or 0 (for no bleeding)
df.bleeding = df.bleeding.apply(lambda x: 1 if x > 0 else 0)

# Make the training and test sets
msk = np.random.rand(len(df)) < 0.75
df_train = df[msk]
df_test = df[~msk]

# Get the predictors
x_train = df_train.drop(columns="bleeding").to_numpy()
x_test = df_test.drop(columns="bleeding").to_numpy()

# Get the outcome column (bleeding)
y_train = df_train.bleeding.to_numpy()
y_test = df_test.bleeding.to_numpy()

normalizer = tf.keras.layers.Normalization(axis=-1)
normalizer.adapt(x_train)

# Neural network model
num_hidden_nodes = x_train.shape[1]
model = tf.keras.models.Sequential(
    [
        normalizer,
        tf.keras.layers.Dense(num_hidden_nodes, activation="relu"),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(num_hidden_nodes, activation="relu"),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(2, activation="softmax"),
    ]
)

loss_fn = tf.keras.losses.SparseCategoricalCrossentropy()
model.compile(optimizer="adam", loss=loss_fn, metrics=["accuracy"])

# Fit and evaluate the model
model.fit(x_train, y_train, epochs=5)
model.evaluate(x_test, y_test, verbose=2)

# Obtain the probabilities of bleeding
log_odds = model(x_test)
probs = tf.nn.softmax(log_odds).numpy()
prob_of_bleeding = probs[:, 1]

# Plot ROC curve
fpr, tpr, threshold = metrics.roc_curve(y_test, prob_of_bleeding)
roc_auc = metrics.auc(fpr, tpr)
plt.title("Receiver Operating Characteristic")
plt.plot(fpr, tpr, "b", label="AUC = %0.2f" % roc_auc)
plt.legend(loc="lower right")
plt.plot([0, 1], [0, 1], "r--")
plt.xlim([0, 1])
plt.ylim([0, 1])
plt.ylabel("True Positive Rate")
plt.xlabel("False Positive Rate")
plt.show()

#!/usr/bin/env python
# coding: utf-8

# In[1]:


# Might need to install dependencies
# pip install tensorflow==1.15.0


# citations: 
# * https://www.kaggle.com/amarpreetsingh/stock-prediction-lstm-using-keras
# * https://docs.scipy.org/doc/numpy/reference/routines.array-manipulation.html
# * https://pythonprogramming.net/recurrent-neural-network-deep-learning-python-tensorflow-keras/
# * https://machinelearningmastery.com/time-series-prediction-lstm-recurrent-neural-networks-python-keras/
# * https://towardsdatascience.com/recurrent-neural-networks-by-example-in-python-ffd204f99470
# * https://towardsdatascience.com/choosing-the-right-hyperparameters-for-a-simple-lstm-using-keras-f8e9ed76f046

# In[2]:


# Library for manipulating, formatting, and cleaning the data
import pandas as pd


# In[3]:


# Load the csv into a data frame
data = pd.read_csv('./input/all_stocks_5yr.csv')


# In[28]:


# See what we have
data


# In[29]:


# (data['Name']=='MMM') returns true/false
# .close says let's just select the column
companyCode = 'MMM'
closeData = data[data['Name']== companyCode].close


# In[30]:


# here is what is in the close array
closeData


# **Now we begin to shape the data for our RNN**

# In[31]:


# Allows us to reshape data, format nicely
import numpy as np


# In[8]:


# Allows us to normalize the data
# All values will now be between 0 and 1
from sklearn.preprocessing import MinMaxScaler


# Adding these "type" lines of code to better understand the transformation of data types
# https://docs.scipy.org/doc/numpy/reference/generated/numpy.reshape.html

# In[9]:


type(closeData)


# In[10]:


type(closeData.values)


# In[11]:


closeData.shape


# In[12]:


closeData = closeData.values.reshape(closeData.shape[0], 1)


# In[13]:


scl = MinMaxScaler()
closeData = scl.fit_transform(closeData)
closeData.shape


# In[14]:


# NOW we have one column with 1259 rows
closeData


# In[15]:


closeData.shape


# - Next step is to preprocess the data
# - Must format it into the shape of our input layer
# - Splitting into chunks of 7 (week)

# In[16]:


# This code is too similar to one of the citations
# Will need to fix/try my own process in the next Sprint
# Create a function to process the data into 7 day look back slices
def processData(data,lb):
    X,Y = [],[]
    for i in range(len(data)-lb-1):
        X.append(data[i:(i+lb),0])
        Y.append(data[(i+lb),0])
    return np.array(X),np.array(Y)


# In[17]:


X,y = processData(closeData,7)
X_train,X_test = X[:int(X.shape[0]*0.80)],X[int(X.shape[0]*0.80):]
y_train,y_test = y[:int(y.shape[0]*0.80)],y[int(y.shape[0]*0.80):]
print(X_train.shape[0])
print(X_test.shape[0])
print(y_train.shape[0])
print(y_test.shape[0])


# # Model Creation

# In[18]:


from keras.models import Sequential
from keras.layers import LSTM,Dense,Dropout


# In[19]:


#Build the model
model = Sequential()
model.add(LSTM(10,input_shape=(7,1)))
model.add(Dropout(0.2))

model.add(Dense(1))
model.compile(optimizer='adam',loss='mse')

#Reshape data for (Sample,Timestep,Features) 
X_train = X_train.reshape((X_train.shape[0],X_train.shape[1],1))
X_test = X_test.reshape((X_test.shape[0],X_test.shape[1],1))


# In[20]:


#Fit model with history to check for overfitting
history = model.fit(X_train,y_train,epochs=200,validation_data=(X_test,y_test),shuffle=False)


# # Plotting Loss function for model

# In[21]:


import matplotlib.pyplot as plt

plt.plot(history.history['loss'])
plt.plot(history.history['val_loss'])


# In[22]:


X_test[0]


# In[23]:


Xt = model.predict(X_test)
plt.plot(scl.inverse_transform(y_test.reshape(-1,1)))
plt.plot(scl.inverse_transform(Xt))


# In[24]:


act = []
pred = []
for i in range(250):
    Xt = model.predict(X_test[i].reshape(1,7,1))
    print('predicted:{0}, actual:{1}'.format(scl.inverse_transform(Xt),scl.inverse_transform(y_test[i].reshape(-1,1))))
    pred.append(scl.inverse_transform(Xt))
    act.append(scl.inverse_transform(y_test[i].reshape(-1,1)))


# In[25]:


result_df = pd.DataFrame({'pred':list(np.reshape(pred, (-1))),'act':list(np.reshape(act, (-1)))})


# In[26]:


X_test[249]


# In[27]:


X_test[248]

import os
os.mkdir('./modelBin/MMM')


model_json = model.to_json()
with open("./modelBin/MMM/model.json", "w+") as json_file:
    json_file.write(model_json)
model.save_weights("./modelBin/MMM/model.h5")

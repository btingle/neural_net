### Neural Net

# Summary

This is a neural network program I wrote from scratch in C++. I wrote it with the intent to make it learn to recognize handwritten digits, inspired by threeblueonebrown's series of videos on the topic. The network I trained learned to recognize the digits with 95% accuracy, and I have included the weights and biases of that network in the repository.

# Overview

The only external library I used for this project was the armadillo linear algebra library. http://arma.sourceforge.net/

Keep in mind that armadillo depends on BLAS and LAPACK libraries for functionality, so if you want to run this project (for some reason) you will need to link them. You can either directly link these libraries, or compile armadillo and link the wrapper library it creates.

# Updates

I kind of burnt out on this project after a while. It works but could definitely use some improvement. 

Examples: 
- Right now you need to manually enter the number of iterations you want the network to train for and wait for it to finish. I would prefer to be able to just set it to train and be able to stop it whenever I felt like it. 

- The learning rate needs to be manually adjusted over time, which is annoying. Training is an arduous process where you continuosuly need to tweak the learning rate each time you train it for an extended number of iterations. There are methods that can be used to adjust the learning rate over time that I would like to eventually implement.

- The command line interface to tweak, train, and test the networks with is pretty jank-y and rushed. In addition, a lot of the file I/O relating to saving and loading the network weights and biases is functional but sub-par IMO.

- The code is configured towards the example I wrote it with in mind. I would like it to be general enough to support the training of any network I have in mind.

- General performance improvements. The network learns fairly quickly, but I'm sure it could be optimized more. Part of this could come from the adaptive learning rate changes I mentioned prior, but there are also a couple others I have in mind.
  * Optimize the main loop. I'm sure there are cycles to cut somewhere!
  * Use GPU acceleration, with the likes of CUDA. CUDA seemed kind of intimidating, so I avoided it in favor of armadillo.
  
Anyways, nobody is going to read any of this. This is just a personal project I've been working on to try and get a better understanding of machine learning.

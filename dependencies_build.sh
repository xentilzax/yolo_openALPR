sudo apt-get install ruby-dev build-essential
sudo gem install fpm

source dependencies.sh

#it just for NVCC
cp /etc/environment ~/environment
sed -i -e 's/\:\/usr\/local\/cuda\/bin//g' ~/environment
sed -i -e 's/\/usr\/local\/cuda\/bin\://g' ~/environment
sed -i -e 's/PATH=\"/PATH=\"\/usr\/local\/cuda\/bin\:/g' ~/environment
sudo mv ~/environment /etc/

sed -i -e 's/export CUDA_HOME=\/usr\/local\/cuda-9.0//g' ~/.bashrc
sed -i -e 's/export CUDA_HOME=\/usr\/local\/cuda-9.1//g' ~/.bashrc
sed -i -e 's/export CUDA_HOME=\/usr\/local\/cuda//g' ~/.bashrc
sed -i -e 's/export LD_LIBRARY_PATH=\$CUDA_HOME\/lib\:\$LD_LIBRARY_PATH//g' ~/.bashrc
sed -i -e 's/export PATH=\$CUDA_HOME\/bin\:\$PATH//g' ~/.bashrc

echo "export CUDA_HOME=/usr/local/cuda" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\$CUDA_HOME/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
echo "export PATH=\$CUDA_HOME/bin:\$PATH" >> ~/.bashrc
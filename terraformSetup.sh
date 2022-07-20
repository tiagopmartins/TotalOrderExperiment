#!/bin/bash

cd terraform-provider-grid5000-totalorder/
cd examples/kubernetes
rm terraform.tfstate
terraform init
printf yes | terraform apply
printf yes | terraform apply
export KUBECONFIG=${PWD}/kube_config_cluster.yml
cp kube_config_cluster.yml ~/.kube/config
#cd ~/promptcc-experiment
#sh ./setup
#cd ~/TccExperimentStorage
#mv *.log ~/promptcc-experiment/results/
#cd ~/promptcc-experiment/results/
#git add *
#git commit -m "added new result"
#git push
#cd ~/terraform-provider-grid5000-promptcc/examples/kubernetes
#printf yes | terraform destroy
#cd ~/promptcc-experiment

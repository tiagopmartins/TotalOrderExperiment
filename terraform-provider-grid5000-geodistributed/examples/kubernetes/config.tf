# Need terraform 0.13
terraform {
  required_providers {
    rke = {
      source = "rancher/rke"
      version = "~> 1.0.1"
    }
    grid5000 = {
      source = "pmorillon/grid5000"
      version = "~> 0.0.5"
    }
    helm = {
      source = "hashicorp/helm"
      version = "~> 1.3.2"
    }
    kubernetes = {
      source = "hashicorp/kubernetes"
      version = "~> 1.13.3"
    }
    localprovider = {
      source = "hashicorp/local"
      version = "~> 2.0.0"
    }
    nullprovider = {
      source = "hashicorp/null"
      version = "~> 3.0.0"
    }
  }
}

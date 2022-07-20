locals {
  nodes_count = 3
  nodes_count_nancy = 2
  nodes_count_rennes = 1
  nodes_count_nantes = 1
  types = ["C", "M", "M", "M"]
  walltime="1:0:0"
}

resource "grid5000_job" "my_job1" {
  name      = "Terraform RKE"
  site      = "nancy"
  command   = "sleep 72h"
  resources = "/nodes=${local.nodes_count_nancy},walltime=${local.walltime}"
  properties = "cluster='gros'"
  types     = ["deploy"]
}

resource "grid5000_job" "my_job2" {
  name      = "Terraform RKE"
  site      = "rennes"
  command   = "sleep 72h"
  resources = "/nodes=${local.nodes_count_rennes},walltime=${local.walltime}"
  properties = "cluster='paravance'"
  types     = ["deploy"]
}

resource "grid5000_job" "my_job3" {
  name      = "Terraform RKE"
  site      = "nantes"
  command   = "sleep 72h"
  resources = "/nodes=${local.nodes_count_nantes},walltime=${local.walltime}"
  properties = "cluster='ecotype'"
  types     = ["deploy"]
}

resource "grid5000_deployment" "my_deployment1" {
  site        = "nancy"
  environment = "debian10-x64-base"
  nodes       = grid5000_job.my_job1.assigned_nodes
  key         = file("~/.ssh/id_rsa.pub")
}

resource "grid5000_deployment" "my_deployment2" {
  site        = "rennes"
  environment = "debian10-x64-base"
  nodes       = grid5000_job.my_job2.assigned_nodes
  key         = file("~/.ssh/id_rsa.pub")
}

resource "grid5000_deployment" "my_deployment3" {
  site        = "nantes"
  environment = "debian10-x64-base"
  nodes       = grid5000_job.my_job3.assigned_nodes
  key         = file("~/.ssh/id_rsa.pub")
}

locals {
  site        = "rennes"
  nodes_count = 23
  nodes_count_rennes = 12
  nodes_count_nancy = 11
  types = ["C", "S", "B", "B", "M", "M", "M", "M", "M", "M", "M", "M", "S", "B", "B", "M", "M", "M", "M", "M", "M", "M", "M"]
}

resource "grid5000_job" "my_job1" {
  name      = "Terraform RKE"
  site      = local.site
  command   = "sleep 24h"
  resources = "/nodes=${local.nodes_count_rennes},walltime=1:0:0"
  properties = "cluster='paravance'"
  types     = ["deploy"]
}

resource "grid5000_job" "my_job2" {
  name      = "Terraform RKE"
  site      = "nancy"
  command   = "sleep 24h"
  resources = "/nodes=${local.nodes_count_nancy},walltime=1:0:0"
  properties = "cluster='gros'"
  types     = ["deploy"]
}

resource "grid5000_deployment" "my_deployment1" {
  site        = local.site
  environment = "debian10-x64-base"
  nodes       = grid5000_job.my_job1.assigned_nodes
  key         = file("~/.ssh/id_rsa.pub")
}

resource "grid5000_deployment" "my_deployment2" {
  site        = "nancy"
  environment = "debian10-x64-base"
  nodes       = grid5000_job.my_job2.assigned_nodes
  key         = file("~/.ssh/id_rsa.pub")
}

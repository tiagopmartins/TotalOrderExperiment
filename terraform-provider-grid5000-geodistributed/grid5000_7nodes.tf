locals {
  site        = "rennes"
  nodes_count = 7
  nodes_count_rennes = 4
  nodes_count_nancy = 3
  types = ["C", "S", "B", "M", "S", "B", "M"]
}

resource "grid5000_job" "my_job1" {
  name      = "Terraform RKE"
  site      = local.site
  command   = "sleep 24h"
  resources = "/nodes=${local.nodes_count_rennes}"
  properties = "cluster='paravance'"
  types     = ["deploy"]
}

resource "grid5000_job" "my_job2" {
  name      = "Terraform RKE"
  site      = "nancy"
  command   = "sleep 24h"
  resources = "/nodes=${local.nodes_count_nancy}"
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

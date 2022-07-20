locals {
  site        = "rennes"
  nodes_count = 74
  nodes_count_rennes = 37
  nodes_count_nancy = 37
  types = ["C", "S", "B", "B", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "S", "B", "B", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M", "M"]
}

resource "grid5000_job" "my_job1" {
  name      = "Terraform RKE"
  site      = local.site
  command   = "sleep 1h"
  resources = "/nodes=${local.nodes_count_rennes}"
  properties = "cluster='paravance'"
  types     = ["deploy"]
}

resource "grid5000_job" "my_job2" {
  name      = "Terraform RKE"
  site      = "nancy"
  command   = "sleep 1h"
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

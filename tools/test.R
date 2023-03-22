library(devtools)
load_all()
try_connect("select top 10 PrimaryProcedure_OPCS from abi.dbo.vw_apc_sem_001")

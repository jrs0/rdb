getwd()
rdb:::try_connect("xsw",
                 "select top 10 PrimaryProcedure_OPCS, DiagnosisPrimary_ICD from abi.dbo.vw_apc_sem_001")

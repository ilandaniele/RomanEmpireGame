<#
.SYNOPSIS
    Retired unsafe prototype entry point.
.DESCRIPTION
    The previous script sent source to an external model, overwrote complete
    files from model output, committed with git add -A, and pushed directly to
    main. M1 recovery deliberately removes that execution path.
#>

[CmdletBinding()]
param()

throw 'Tools/dev_orchestrator.ps1 is retired. Use IMPLEMENTATION_PLAN.md, Tools/validate_repository.ps1, and BuildProject.ps1 through reviewed changes.'

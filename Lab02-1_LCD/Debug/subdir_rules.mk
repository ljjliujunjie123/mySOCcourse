################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
dr_lcdseg.obj: ../dr_lcdseg.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"D:/some_useful_program/ccs/ccsv5/tools/compiler/msp430_4.1.2/bin/cl430" -vmspx --abi=coffabi -g --include_path="D:/some_useful_program/ccs/ccsv5/ccs_base/msp430/include" --include_path="D:/some_useful_program/ccs/ccsv5/tools/compiler/msp430_4.1.2/include" --define=__MSP430F6638__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="dr_lcdseg.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"D:/some_useful_program/ccs/ccsv5/tools/compiler/msp430_4.1.2/bin/cl430" -vmspx --abi=coffabi -g --include_path="D:/some_useful_program/ccs/ccsv5/ccs_base/msp430/include" --include_path="D:/some_useful_program/ccs/ccsv5/tools/compiler/msp430_4.1.2/include" --define=__MSP430F6638__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


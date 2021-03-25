################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"D:/Ti/ccs1020/ccs/tools/compiler/msp430_4.1.2/bin/cl430" -vmspx --abi=coffabi --data_model=restricted -g --include_path="D:/Ti/ccs1020/ccs/ccs_base/msp430/include" --include_path="D:/Ti/ccs1020/ccs/tools/compiler/msp430_4.1.2/include" --define=__MSP430F6638__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



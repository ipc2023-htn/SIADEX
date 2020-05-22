#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <vector>

#include "cwa.hpp"
#include "domain.hpp"
#include "hddl.hpp"
#include "hddlWriter.hpp"
#include "hpdlWriter.hpp"
#include "output.hpp"
#include "parametersplitting.hpp"
#include "parsetree.hpp"
#include "plan.hpp"
#include "shopWriter.hpp"
#include "sortexpansion.hpp"
#include "typeof.hpp"
#include "util.hpp"
#include "verify.hpp"
#include "properties.hpp"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE* f, char* filename);

// parsed domain data structures
bool has_typeof_predicate = false;
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;
vector<parsed_task> parsed_primitive;
vector<parsed_task> parsed_abstract;
map<string,vector<parsed_method> > parsed_methods;
vector<pair<predicate_definition,string>> parsed_functions;
string metric_target = dummy_function_type;


map<string,set<string> > sorts;
vector<method> methods;
vector<task> primitive_tasks;
vector<task> abstract_tasks;

map<string, task> task_name_map;


int main(int argc, char** argv) {
	cin.sync_with_stdio(false);
	cout.sync_with_stdio(false);
	int dfile = -1;
	int pfile = -1;
	int doutfile = -1;
	int poutfile = -1;
	bool splitParameters = true;
	bool compileConditionalEffects = true;
	bool linearConditionalEffectExpansion = false;
	bool encodeDisjunctivePreconditionsInMethods = false;
	bool compileGoalIntoAction = false;
	
	bool shopOutput = false;
	bool hpdlOutput = false;
	bool hddlOutput = false;
	bool internalHDDLOutput = false;
	bool lenientVerify = false;
	bool verboseOutput = false;
	bool verifyPlan = false;
	bool useOrderInPlanVerification = true;
	bool convertPlan = false;
	bool showProperties = false;
	int verbosity = 0;
	
	struct option options[] = {
		{"no-split-parameters"                    , no_argument,       NULL,   's'},
		{"keep-conditional-effects"               , no_argument,       NULL,   'k'},
		{"linear-conditional-effect"              , no_argument,       NULL,   'L'},
		{"encode-disjunctive_preconditions-in-htn", no_argument,       NULL,   'D'},
		{"compile-goal"							  , no_argument,       NULL,   'g'},
		
		{"shop"                                   , no_argument,       NULL,   'S'},
		{"shop2"                                  , no_argument,       NULL,   'S'},
		{"shop1"                                  , no_argument,       NULL,   '1'},
		{"hpdl"                                   , no_argument,       NULL,   'H'},
		{"hddl"                                   , no_argument,       NULL,   'h'},
		{"hddl-internal"                          , no_argument,       NULL,   'i'},
		
		{"panda-converter"                        , no_argument,       NULL,   'c'},
		{"verify"                                 , optional_argument, NULL,   'v'},
		{"vverify"                                , no_argument,       NULL,   'V'},
		{"vvverify"                               , no_argument,       NULL,   'W'},
		{"lenient"                                , no_argument,       NULL,   'l'},
		{"verify-no-order"                        , no_argument,       NULL,   'o'},
		
		{"no-color"                               , no_argument,       NULL,   'C'},
		{"debug"                                  , optional_argument, NULL,   'd'},
		
		{"properties"                             , optional_argument, NULL,   'p'},
		
		{NULL                                     , 0,                 NULL,   0},
	};

	bool optionsValid = true;
	while (true) {
		int c = getopt_long_only (argc, argv, "sS1HcvVWoCdkhilpLDg", options, NULL);
		if (c == -1)
			break;
		if (c == '?' || c == ':'){
			// Invalid option; getopt_long () will print an error message
			optionsValid = false;
			continue;
		}

		if (c == 's') splitParameters = false;
		else if (c == 'k') compileConditionalEffects = false;
		else if (c == 'L') { compileConditionalEffects = false; linearConditionalEffectExpansion = true; }
		else if (c == 'D') encodeDisjunctivePreconditionsInMethods = true;
		else if (c == 'g') compileGoalIntoAction = true;
		else if (c == 'S') shopOutput = true;
		else if (c == '1') { shopOutput = true; shop_1_compatability_mode = true; }
	   	else if (c == 'H') hpdlOutput = true;
	   	else if (c == 'h') hddlOutput = true;
	   	else if (c == 'i') { hddlOutput = true; internalHDDLOutput = true; }
		else if (c == 'c') convertPlan = true;
		else if (c == 'v') {
			verifyPlan = true;
			if (optarg) verbosity = atoi(optarg);
		} else if (c == 'V') { verifyPlan = true; verbosity = 1; }
		else if (c == 'W') { verifyPlan = true; verbosity = 2; }
		else if (c == 'o') { verifyPlan = true; useOrderInPlanVerification = false; }
		else if (c == 'l') { verifyPlan = true; lenientVerify = true; }
		else if (c == 'C') no_colors_in_output = true;
		else if (c == 'p') showProperties = true;
		else if (c == 'd') {
		   	verboseOutput = true;
			if (optarg) verbosity = atoi(optarg);
		}
	}
	if (!optionsValid) {
		cout << "Invalid options. Exiting." << endl;
		return 1;
	}

	for (int i = optind; i < argc; i++) {
		if (dfile == -1) dfile = i;
		else if (pfile == -1) pfile = i;
		else if (doutfile == -1) doutfile = i;
		else if (poutfile == -1) poutfile = i;
	}

	if (dfile == -1){
		if (convertPlan)
			cout << "You need to provide a plan as input." << endl;
		else
			cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// if we want to simplify a plan, just parse nothing
	if (convertPlan){
		ifstream * plan   = new ifstream(argv[dfile]);
		ostream * outplan = &cout;
		if (pfile != -1){
			ofstream * of  = new ofstream(argv[pfile]);
			if (!of->is_open()){
				cout << "I can't open " << argv[pfile] << "!" << endl;
				return 2;
			}
			outplan = of;
		}
		
		
		convert_plan(*plan, *outplan);
		return 0;
	}

	if (pfile == -1 && !convertPlan){
		cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// open c-style file handle 
	FILE *domain_file = fopen(argv[dfile], "r");
	FILE *problem_file = fopen(argv[pfile], "r");

	if (!domain_file) {
		cout << "I can't open " << argv[dfile] << "!" << endl;
		return 2;
	}
	if (!problem_file) {
		cout << "I can't open " << argv[pfile] << "!" << endl;
		return 2;
	}
	if (!shopOutput && !hpdlOutput && !hddlOutput && poutfile != -1){
		cout << "For ordinary pandaPI output, you may only specify one output file, but you specified two: " << argv[doutfile] << " and " << argv[poutfile] << endl;
	}
	
	// parsing of command line arguments has been completed	
		
		
	// parse the domain file
	run_parser_on_file(domain_file, argv[dfile]);
	run_parser_on_file(problem_file, argv[pfile]);

	if (showProperties){
		printProperties();
		return 0;
	}

	if (!hpdlOutput) expand_sorts(); // add constants to all sorts
	
	// handle typeof-predicate
	if (!hpdlOutput && has_typeof_predicate) create_typeof();

	if (compileGoalIntoAction) compile_goal_into_action();


	// do not preprocess the instance at all if we are validating a solution
	if (verifyPlan){
		ifstream * plan  = new ifstream(argv[doutfile]);
		bool result = verify_plan(*plan, useOrderInPlanVerification, lenientVerify, verbosity);
		cout << "Plan verification result: ";
		if (result) cout << color(COLOR_GREEN,"true",MODE_BOLD);
		else cout << color(COLOR_RED,"false",MODE_BOLD);
		cout << endl;
		return 0;
	}

	if (!hpdlOutput) {
		// flatten all primitive tasks
		flatten_tasks(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
		// .. and the goal
		flatten_goal();
		// create appropriate methods and expand method preconditions
		parsed_method_to_data_structures(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
	}

	if (shopOutput || hpdlOutput){
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(argv[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << argv[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(argv[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << argv[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (shopOutput)	write_instance_as_SHOP(*dout,*pout);
		if (hpdlOutput)	write_instance_as_HPDL(*dout,*pout);
		return 0;
	}


	// split methods with independent parameters to reduce size of grounding
	if (splitParameters) split_independent_parameters();
	// cwa, but only if we actually want to compile negative preconditions
	if (!hpdlOutput || internalHDDLOutput) compute_cwa();
	// simplify constraints as far as possible
	reduce_constraints();
	clean_up_sorts();
	remove_unnecessary_predicates();

	// write to output
	if (verboseOutput) verbose_output(verbosity);
	else if (hddlOutput) {
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(argv[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << argv[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(argv[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << argv[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		hddl_output(*dout,*pout, internalHDDLOutput);
	} else {
		ostream * dout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(argv[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << argv[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		simple_hddl_output(*dout);
	}
}

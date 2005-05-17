(*
 * Core of a genetic algorithm
 * Copyright 2005 Robert Ricci for the University of Utah
 * ricci@cs.utah.edu, testbed-ops@emulab.net
 *)

(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 *)

type individual = int array;;

(* Parameters to the GA - see the default_opts below *)
type ga_options = {
    fitness_function : (individual -> float);
    mutation_function : (individual -> individual);
    crossover_function : (individual -> individual -> individual);
    pop_size : int;
    breeder_size : int;
    crossover_pct : float;
    verbose : bool;
    termination_score : float;
    rand_seed : int;
    elite_pct : float
};;

(* Simple mutation for permutation encoding *)
let permutation_mutation (parent : individual) : individual =
    let child = Array.copy parent in
    let ngenes = Array.length child in
    let gene1 = Random.int ngenes in
    let gene2 = Random.int ngenes in
    let tmp = child.(gene1) in
    child.(gene1) <- child.(gene2);
    child.(gene2) <- tmp;
    child
;;


(* Simple crossover for permutation encoding *)
let permutation_crossover (mom : individual) (dad : individual) : individual =
    let ngenes = Array.length mom in
    let child = Array.create ngenes 65535 in (* XXX - we have to supply a default *)
    let child_contains_gene (gene : int) : bool =
        let rec contain_helper (i : int) : bool =
            if i == (Array.length child) then
                false
            else
                (child.(i) = gene) || contain_helper (i + 1)
        in
        contain_helper 0 
    in
    let crosspoint = Random.int ngenes in
    for i = 0 to crosspoint do
        child.(i) <- mom.(i)
    done;
    let childindex = ref (crosspoint + 1) in
    (* TODO - this certainly could be done in a more efficient manner *)
    for i = 0 to (ngenes - 1) do
        if not (child_contains_gene dad.(i)) then begin
            child.(!childindex) <- dad.(i);
            childindex :=  !childindex + 1
        end
    done;
    child
;;

(* Some null functions for testing *)
let null_crossover (mom : individual) (dad : individual) : individual =
    mom
;;

let null_fitness (ind : individual) : float =
    0.0
;;

(* A fitness function that returns the number of elements whose cell contents
 * equal its index *)
let ordered_fitness (ind : individual) : float =
    let rec ordered_helper (i : int) : int =
        if i >= (Array.length ind) then
            0
        else
            (if ind.(i) = i then 1 else 0) + ordered_helper (i + 1)
    in
    (float_of_int (ordered_helper 0)) /. (float_of_int (Array.length ind))
;;

(* Reverse the order of the regular compare function *)
let desc_compare x y =
    match x with (x1,x2) -> match y with (y1,y2) ->
    Pervasives.compare y1 x1
;;


type poplist = (float * individual) list;;
exception GAError of string;;

let rec select_first_n (l : 'a list) (n : int) : 'a list =
    if n = 0 then [] else
    match l with 
      [] -> []
    | x :: xs ->
            x :: select_first_n xs (n - 1)
;;

let rec split_after_n (l : 'a list) (n : int) : ('a list * 'a list) =
    if n = 0 then ([],l) else
    match l with 
      [] -> ([],[])
    | x :: xs ->
        match (split_after_n xs (n - 1)) with (y,ys) ->
            (x :: y, ys)
;;

(*
 * Simple roulette selector - does sampling with replacement, so it could
 * end up selecting an individual more than once. This makes it much more
 * efficient, though
 *)
let roulette_select (candidates : poplist) (howmany : int) : poplist =
    (* Simple optimization *)
    if (List.length candidates) <= howmany then candidates else
    let accum_score (accum : float) (thing : (float * 'a)) : float =
        match thing with (score,_) -> accum +. score in
    let score_sum = List.fold_left accum_score 0.0 candidates in
    let rec get_individual (l : poplist) (s : float) : (float * individual) =
        match l with
          x :: [] -> x
        | x :: xs -> (match x with (score,individual) ->
              if score <= s then
                  (score, individual)
              else
                  get_individual xs (s -. score))
        | [] -> raise (Failure "Empty population in roulette_select")
    in
    let rec get_n_individuals (i : int) : poplist =
        let target_score = Random.float score_sum in 
        if i = 0 then [] else
        (get_individual candidates target_score) :: (get_n_individuals (i - 1))
    in
    get_n_individuals howmany
;;

let select_for_breeding (candidates : poplist) (howmany : int) : poplist =
    (* select_first_n candidates howmany *)
    roulette_select candidates howmany
;;


let select_for_survival (candidates : poplist) (howmany : int) (elite_pct : float) : poplist =
    (* Elitism - the best few survive, no matter what *)
    let num_elites = int_of_float((float_of_int howmany) *. elite_pct) in
    let (elites, plebs) = split_after_n candidates num_elites in
    elites @ List.fast_sort desc_compare (roulette_select plebs (howmany - num_elites))
;;

(* It's better if this list passed to this function is _not_ in sorted order! *)
let split_breeders (candidates : poplist) (percentage : float) : (poplist * poplist) =
    let splitpoint = int_of_float ((float_of_int (List.length candidates)) *.  percentage) in
    split_after_n candidates splitpoint
;;

(* It's better if this list passed to this function is _not_ in sorted order! *)
let rec pair_off (cross : (individual -> individual -> individual))
             (parents : individual list) : individual list =
    (* TODO: Mate each one more than once? *)
    match parents with
        mom :: dad :: others -> (cross mom dad) :: (pair_off cross others)
    | _ -> []
;;

(* Default options for the GA *)
let default_opts = {
    fitness_function = ordered_fitness;
    mutation_function = permutation_mutation;
    crossover_function = permutation_crossover;
    pop_size = 50;
    breeder_size = 30;
    crossover_pct = 0.8;
    verbose = false;
    termination_score = 0.99;
    rand_seed = 0;
    elite_pct = 0.05
};;


(*
 * Do the actual optimization!
 *)
exception GADone of individual * int;;
let optimize (initial_pop : individual list) (opts : ga_options) : (individual * int) =
    (* Initialize the PRNG *)
    if opts.rand_seed = 0 then
        Random.self_init ()
    else
        Random.init opts.rand_seed;
    (* Compute fitness for an individual *)
    let calc_fitness (ind : individual) : (float * individual) =
        (opts.fitness_function ind, ind) in
    let pop = List.fast_sort desc_compare (List.map calc_fitness initial_pop) in
    (* This function "returns" by raising an exception with the champion *)
    let rec run_ga (pop : poplist) (generations : int) : (individual * int) =
        match List.hd pop with (score, champion) ->
            if opts.verbose then
                print_endline ("Best this generation: " ^ (string_of_float score) ^
                " of " ^ (string_of_int (List.length pop)));
            if score >= opts.termination_score then raise (GADone (champion, generations));
            let breeders = select_for_breeding pop opts.breeder_size in
            let (lparents,lmutators) = split_breeders breeders opts.crossover_pct in
            let (_,parents) = List.split lparents in 
            let (_,mutators) = List.split lmutators in 
            let children = List.map calc_fitness (pair_off opts.crossover_function parents) in
            let mutants = List.map calc_fitness (List.map opts.mutation_function mutators) in
            let nextgen = select_for_survival
            (List.fast_sort desc_compare (List.rev_append (List.rev_append pop
            children) mutants)) opts.pop_size opts.elite_pct in
            run_ga nextgen (generations + 1)
    in
    try run_ga pop 1
    with GADone(champion, generations) -> (champion, generations)
;;

(* Some test populations *)
let test_pop = [ [| 4; 1 |]; [| 1; 0 |]; ];;
let test_pop2 = [ [| 10; 3; 13; 0; 16; 11; 2; 12; 4; 17; 15; 5; 14; 1; 6; 9; 8; 7; |]; ];;
let big_pop_size = 300 in
let big_test_array = Array.create big_pop_size 0 in
for i = 0 to big_pop_size - 1 do
    (* Start in reverse order *)
    big_test_array.(i) <- big_pop_size - i - 1
done;
let big_test_pop = [ big_test_array ] in

let (ubermensch, gens) = optimize big_test_pop default_opts in
(* Array.iter (fun x -> print_endline (string_of_int x)) ubermensch *)
print_endline ("Found solution after " ^ (string_of_int gens) ^ " generations")

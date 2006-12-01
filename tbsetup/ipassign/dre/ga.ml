(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * Core of a genetic algorithm
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
    termination_score : float option;
    max_generations : int option;
    rand_seed : int;
    elite_pct : float;
    score_comparator : float -> float -> int;
    victory_generations : int option
};;

(*
 * Use the Mersenne Twister from mathlib to make a random number generator for
 * ints
 *)
(* module MyRNG = Rand.UniformDist2(Math.IntOps)(MtRand.IntSource);;
   let myrand = new MyRNG.rng 0 100 in print_int myrand#genrand; print_newline ();; *)

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

(*
 * Randomize an individual - useful for starting configurations
 *)
let permutation_randomize (initial : individual) : individual =
    let mutations = (Array.length initial) * 2 in
    let rec randomize (ind : individual) (n : int) : individual =
        if n > mutations then ind else
            randomize (permutation_mutation ind) (n + 1)
    in
    randomize initial 0
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

(* Compare the first element of a two-tuple list *)
let first_compare comparator x y =
    match x with (x1,_) -> match y with (y1,_) ->
    comparator x1 y1
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

let rank_select (candidates : poplist) (howmany : int) : poplist =
    (* Simple optimization *)
    if (List.length candidates) <= howmany then candidates else
    let rec sum_ranks (i : int) (l : 'a list) : int =
        match l with
          [] -> 0
        | x :: xs -> i + (sum_ranks (i + 1) xs) in
    let clen = List.length candidates in
    let rec select_individual (target_rank : int) (current_rank : int)
        (candidates : poplist) : (float * individual) =
        match candidates with
          x :: [] -> (print_endline "Case 1"; x)
        | x :: xs -> if target_rank <= 0 then (
            (*print_endline ("Case 2: len=" ^ (string_of_int (List.length
            candidates)) ^ " cr=" ^  (string_of_int  current_rank) ^ " tr=" ^
            (string_of_int target_rank) ^ " abs=" ^ (string_of_int (clen -
            current_rank + 1)));*) x) else
            select_individual (target_rank - current_rank) (current_rank - 1) xs
        | [] -> raise (Failure "Empty population in rank_select")
    in
    let rank_sum = sum_ranks 1 candidates in
    let rec get_n_individuals (i : int) : poplist =
        let target_rank = Random.int rank_sum in
        (*print_endline ("Selecting " ^ (string_of_int target_rank) ^ " of " ^
            (string_of_int rank_sum) ^ " - list size " ^ (string_of_int
            (List.length candidates))); *)
        if i = 0 then [] else
        (select_individual (target_rank - (List.length candidates)) (List.length candidates) candidates)
            :: (get_n_individuals (i - 1))
    in
    get_n_individuals howmany
;;

let select_for_breeding (candidates : poplist) (howmany : int) : poplist =
    (* select_first_n candidates howmany *)
    (* roulette_select candidates howmany *)
    rank_select candidates howmany
;;


let select_for_survival (comparator : float -> float -> int) (candidates : poplist)
        (howmany : int) (elite_pct : float) : poplist =
    (* Elitism - the best few survive, no matter what *)
    let num_elites = int_of_float((float_of_int howmany) *. elite_pct) in
    let (elites, plebs) = split_after_n candidates num_elites in
    elites @ List.fast_sort (first_compare comparator)
        (* (roulette_select plebs (howmany - num_elites)) *)
        (rank_select plebs (howmany - num_elites))
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
    breeder_size = 50;
    crossover_pct = 0.8;
    verbose = false;
    termination_score = Some 0.99;
    max_generations = None;
    rand_seed = 0;
    elite_pct = 0.05;
    (* Default to desceding, meaning high scores are good *)
    score_comparator = (fun x y -> Pervasives.compare y x);
    victory_generations = None
};;


(*
 * Do the actual optimization!
 *)
exception GADone of individual * int;;
let optimize (initial_pop : individual list) (opts : ga_options)
             : (individual * int) =
    (* Initialize the PRNG *)
    if opts.rand_seed = 0 then
        Random.self_init ()
    else
        Random.init opts.rand_seed;
    (* Compute fitness for an individual *)
    let calc_fitness (ind : individual) : (float * individual) =
        (opts.fitness_function ind, ind) in
    let pop = List.fast_sort (first_compare opts.score_comparator)
                             (List.map calc_fitness initial_pop) in
    (* This function "returns" by raising an exception with the champion *)
    let rec run_ga (pop : poplist) (generations : int) (king_score : float)
                   (reign_period : int) : (individual * int) =
        let breeders = select_for_breeding pop opts.breeder_size in
        let (lparents,lmutators) = split_breeders breeders opts.crossover_pct in
        let (_,parents) = List.split lparents in 
        let (_,mutators) = List.split lmutators in 
        let children = List.map calc_fitness (pair_off opts.crossover_function parents) in
        let mutants = List.map calc_fitness (List.map opts.mutation_function mutators) in
        let nextgen = select_for_survival opts.score_comparator
            (List.fast_sort (first_compare opts.score_comparator)
            (List.rev_append (List.rev_append pop
                children) mutants)) opts.pop_size opts.elite_pct in
        let (topscore,champion) = List.hd nextgen in
        if opts.verbose then
            output_string stderr ("Best of generation " ^ (string_of_int generations)
            ^ ": " ^ (string_of_float topscore) ^
            " of " ^ (string_of_int (List.length pop)) ^ "\n");
        (* Termination conditions *)
        match opts.max_generations with
            Some(x) when generations >= x -> raise (GADone (champion, generations))
        | _ ->
        match opts.termination_score with
          Some(x) when topscore >= x -> raise (GADone (champion, generations))
        | _ ->
        let new_reign_period =
            if topscore == king_score then reign_period + 1 else 0 in
        match opts.victory_generations with
          Some(x) when reign_period == x -> raise (GADone (champion, generations))
        | _ ->
        run_ga nextgen (generations + 1) topscore new_reign_period
    in
    try run_ga pop 1 0.0 0
    with GADone(champion, generations) -> (champion, generations)
;;

(* Some test populations *)
(*
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
*)

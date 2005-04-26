(*
 * Core of a genetic algorithm
 * Copyright 2005 Robert Ricci for the University of Utah
 * ricci@cs.utah.edu, testbed-ops@emulab.net
 *)

type individual = int array;;

(* If we get a child this good, we stop - should be a parameter *)
let threshold = 0.99;;

Random.self_init ();;

(* Simple mutation for an array *)
let sample_mutation (parent : individual) : individual =
    let child = Array.copy parent in
    let ngenes = Array.length child in
    let gene1 = Random.int ngenes in
    let gene2 = Random.int ngenes in
    let tmp = child.(gene1) in
    child.(gene1) <- child.(gene2);
    child.(gene2) <- tmp;
    child
;;

let null_crossover (mom : individual) (dad : individual) : individual =
    mom
;;

let null_fitness (ind : individual) : float =
    0.0
;;

let lame_fitness (ind : individual) : float =
    float_of_int ind.(0)
;;

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
exception GADone of individual;;

let rec select_first_n (l : 'a list) (n : int) : 'a list =
    if n = 0 then [] else
    match l with 
      [] -> []
    | x :: xs ->
            x :: select_first_n xs (n - 1)
;;

let select_for_breeding (candidates : poplist) : poplist =
    select_first_n candidates 10
;;

let select_for_survival (candidates : poplist) : poplist =
    select_first_n candidates 20
;;

let optimize (initial_pop : individual list)
             (fitness_function : (individual -> float))
             (mutation_function : (individual -> individual))
             (crossover_function : (individual -> individual -> individual))
             : individual =
    (* Compute fitness for an individual *)
    let calc_fitness (ind : individual) : (float * individual) =
        (fitness_function ind, ind) in
    let pop = List.fast_sort desc_compare (List.map calc_fitness initial_pop) in
    (* This function "returns" by raising an exception with the champion *)
    let rec run_ga (pop : poplist) : individual =
        match List.hd pop with (score, champion) ->
            print_endline ("Best this generation: " ^ (string_of_float score) ^
            " of " ^ (string_of_int (List.length pop)));
            if score >= threshold then raise (GADone champion) else
                let (_,breeders) = List.split (select_for_breeding pop) in
                let children = List.map calc_fitness (List.map mutation_function breeders) in
                let nextgen = select_for_survival
                    (List.fast_sort desc_compare (List.rev_append pop
                        children)) in
                run_ga nextgen
    in
    try run_ga pop
    with GADone(champion) -> champion
;;

let test_pop = [ [| 4; 1 |]; [| 1; 0 |]; ];;
let test_pop2 = [ [| 10; 3; 13; 0; 16; 11; 2; 12; 4; 17; 15; 5; 14; 1; 6; 9; 8; 7; |]; ];;

let ubermensch = optimize test_pop2 ordered_fitness sample_mutation null_crossover in
Array.iter (fun x -> print_endline (string_of_int x)) ubermensch

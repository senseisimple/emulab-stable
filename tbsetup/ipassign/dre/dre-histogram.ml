(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * test-dijk.ml
 * Test functions for my Dijkstra's shortest path implementation
 *)

let samples = ref None;;
let range = ref false;;

let argspec =
  [ "-s", Arg.Int (fun s -> samples := Some s),
      "Number of samples to take in the DRE calculation"; 
    "-r", Arg.Unit (fun () -> range := true),
      "Get DRE range for each node"; 
  ];;

let rec compute_all_dre (g : ('a, 'b) Graph.t)  =
    let hops = Array.make_matrix (Graph.count_nodes g) (Graph.count_nodes g) Dijkstra.NoHop in
    let fill_array (base : unit) (node : (int, 'a) Graph.node) : unit =
        let node_id = node.Graph.node_contents in
        match (Dijkstra.run_dijkstra g node) with (_,pred) ->
        hops.(node_id) <- Dijkstra.get_first_hops g pred node;
        base
    in
    Graph.fold_nodes g fill_array ();
    Dre.compute_dre ~samples:!samples hops
    (*;
    (* let samples = int_of_float (sqrt (float_of_int (Graph.count_nodes g))) in
* *)
    Dre.list_of_dre_matrix (Dre.compute_dre ~samples:!samples hops) *)
;;

(*
exception NeedArg;;
if Array.length Sys.argv < 2 then raise NeedArg;;

let edges = read_graph_file Sys.argv.(1) in
*)
let edgefile = ref None;;
Arg.parse argspec (fun x -> edgefile := Some x) "";;
let edgestr = match !edgefile with
              None -> raise (Failure "Need an arg")
            | Some(x) -> x in
let g = Graph.read_graph_file edgestr in
let dre_array = compute_all_dre g in
let dre_list = if !range then
    let l = ref [] in
    for i = 0 to ((Array.length dre_array) - 1) do
        match Dre.find_min_max dre_array i with
        (min,max) -> l := (max -. min) :: !l
    done;
    !l
else
    Dre.list_of_dre_matrix dre_array
in
Histogram.print_histogram (Histogram.make_histogram dre_list
                            (Histogram.fixed_size_steps 0.0 0.01 101))

(*
for i = 0 to ((Array.length dre_array) - 1) do
    match Dre.find_min_max dre_array i with
    (min,max) -> print_endline (string_of_float (max -. min))
done
let dre_list = compute_all_dre g in
Histogram.print_histogram (Histogram.make_histogram dre_list
                            (Histogram.fixed_size_steps 0.0 0.01 101))
*)

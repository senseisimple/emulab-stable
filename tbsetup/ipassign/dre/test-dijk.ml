(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * test-dijk.ml
 * Test functions for my Dijkstra's shortest path implementation
 *)

let rec print_weights (channel : out_channel) (weights : int array)
                      (index : int) : unit =
    if index >= Array.length weights then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        weights.(index) ^ "\n");
        print_weights channel weights (index + 1))
;;

let rec print_pred (channel : out_channel) (preds : (int, 'b) Graph.node array)
                   (index : int) : unit =
    if index >= Array.length preds then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        preds.(index).Graph.node_contents ^ "\n");
        print_pred channel preds (index + 1))
;;

let rec print_fhop (channel : out_channel) (fhops : int array)
                   (index : int) : unit =
    if index >= Array.length fhops then ()
    else (output_string channel (string_of_int index ^ ": " ^ (string_of_int fhops.(index)));
    (* (match fhops.(index) with
      Dijkstra.NoHop -> "X"
    | Dijkstra.NodeHop(x) -> (string_of_int x.Graph.node_contents)) ^ "\n");
    *)
    print_fhop channel fhops (index + 1))
;;

let rec dijk_all_nodes (g : ('a, 'b) Graph.t) (nodes : ('a, 'b) Graph.node list)
: unit =
    match nodes with
      [] -> ()
    | (x :: xs) ->
            (* print_endline ("On " ^ string_of_int x.Graph.node_contents); *)
            match Dijkstra.run_dijkstra g x with (_,pred) ->
            (* XXX - return this somehow *)
            let fhops = Dijkstra.get_first_hops g pred x in
            print_endline ("FHops for " ^ (string_of_int x.Graph.node_contents));
            print_fhop stdout fhops 0;
            (*
            let res = Dijkstra.run_dijkstra g x in
            match res with (weights,_) -> print_weights stdout weights 0; *)
            dijk_all_nodes g xs
;;

exception NeedArg;;
if Array.length Sys.argv < 2 then raise NeedArg;;
(* print_endline "Here 1"; *)

let g = Graph.read_graph_file Sys.argv.(1) in
(* print_endline "Here 3"; *)
let node = Graph.find_node g 0 in
(* print_endline "Here 4"; *)
let _ = dijk_all_nodes g g.Graph.nodes in
();
(* print_endline "Here 5"; *)
(*
let res = Dijkstra.run_dijkstra g node in
match res with (weights,preds) ->
(* print_weights stdout weights 0; *)
print_pred stdout preds 0;;
*)

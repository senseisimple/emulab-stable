(*
 * graph2dot.ml - Convert Jon Duerig's graph files into input files for dot
 * File format is a bunch of lines constisting of four ints where:
 * First is edge weight (unused)
 * Second is unused
 * Third is source vertex number
 * Fourth is destination vertex number
 * ... verticies exist only implicitly, as referenced by edges.
 *)

(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(* Hmm, this is awkward, I have to declare all of these types even though they
 * are related. There's probably some better way to do this *)
type mygraph = (int, unit) Graph.t;;
type mynode = (int, unit) Graph.node;;
type myedge = (int, unit) Graph.edge;;

type edge = int * int;;

(* Get a list of edges from a channel *)
let rec (get_edges : in_channel -> edge list) = function channel ->
    try
        let line = input_line channel in
        let parts = Str.split (Str.regexp " +") line in
        (int_of_string (List.nth parts 2), int_of_string (List.nth parts 3))
            :: get_edges channel
    with
        End_of_file -> []
;;

(* Read in one of Jon's graph files *)
let (read_graph_file : string -> edge list) = function filename ->
    let channel = open_in filename in
    get_edges channel
;;

(* Make a graph from and edge_list *)
let rec (make_graph_from_edges : edge list -> mygraph) = function edges ->
    match edges with
      [] -> Graph.empty_graph()
    | x::xs -> let g = make_graph_from_edges xs in
        (match x with (first, second) -> 
            (* Add the verticies to the graph if they are not in there
             * already *)
            let src =
                if not (Graph.is_member g first) then Graph.add_node g first
                    else Graph.find_node g first in
            let dst =
                if not (Graph.is_member g second) then Graph.add_node g second
                    else Graph.find_node g second in
            let edge = Graph.add_edge g src dst () in
        g)
;;

let (dot_print_vertex : out_channel -> mynode -> unit) =
        function channel -> function vertex ->
    output_string channel ("\t" ^ (string_of_int vertex.Graph.node_contents)
        ^ " [color=red" ^ ",style=filled,shape=point];\n")
;;

let (dot_print_edge : out_channel -> myedge -> unit) =
        function channel -> function edge ->
    output_string channel ("\t" ^ (string_of_int
        (edge.Graph.src.Graph.node_contents)) ^ " -> " ^ (string_of_int
        (edge.Graph.dst.Graph.node_contents)) ^
        " [arrowhead=none,color=black];\n")
;;

let (dot_print : mygraph -> string -> unit) = function g -> function filename ->
    let channel = open_out filename in
    (* Preamble *)
    output_string channel "digraph foo {\n";
    output_string channel "\tnodesep=0.01\n";
    output_string channel "\tranksep=0.5\n";
    Graph.iterate_nodes g (dot_print_vertex channel);
    Graph.iterate_edges g (dot_print_edge channel);
    (* Prologue *)
    output_string channel "}\n";
    close_out channel
;;

(* Hooray for instant gratification! *)
let show g =
    let tmp = Filename.temp_file "graph" ".dot" in
    let tmp2 = Filename.temp_file "graph" ".ps" in
    (* dot_output g tmp; *)
    dot_print g tmp;
    ignore (Sys.command ("dot -Tps < " ^ tmp ^ " > " ^ tmp2 ^ " && gv " ^ tmp2));
    Sys.remove tmp;
    Sys.remove tmp2
;;

let edges = read_graph_file "test.graph" in
let g = make_graph_from_edges edges in
dot_print g "graph.dot"

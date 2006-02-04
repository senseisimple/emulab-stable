(*
 * graph2dot.ml - Convert Jon Duerig's graph files into input files for dot
 * File format is a bunch of lines constisting of four ints where:
 * First is edge weight (unused)
 * Second is unused
 * Third is source vertex number
 * Fourth is destination vertex number
 * ... verticies exist only implicitly, as referenced by edges.
 *)

let namemap = Hashtbl.create 127;;

let rec (fill_hash_map : in_channel -> int -> unit) = function channel ->
    function i ->
    try
        let line = input_line channel in
        let parts = Str.split (Str.regexp " +") line in
        let mapped_to = int_of_string (List.nth parts 0) in
        (* print_endline ("Mapping " ^ (string_of_int i) ^ " to " ^ (string_of_int
        mapped_to)); *)
        Hashtbl.add namemap i mapped_to;
        fill_hash_map channel (i + 1)
    with
        End_of_file -> ()
;;

let (fill_map : string -> unit) = function filename ->
    let channel = open_in filename in
    fill_hash_map channel 0
;;

let (dot_print_vertex : out_channel -> ('a,'b) Graph.node -> unit) =
        function channel -> function vertex ->
    let node_id = vertex.Graph.node_contents in
    let name = if Hashtbl.mem namemap node_id then ((*print_endline "Mapped"; *) Hashtbl.find namemap node_id)
    else node_id in
    let str_name = string_of_int name in
    output_string channel ("\t" ^ (string_of_int node_id)
        ^ " [color=red,label=\"" ^ str_name ^
        "\",style=filled,shape=plaintext,fontcolor=black,height=.01,width=.01,fontsize=8];\n")
;;

let (dot_print_edge : out_channel -> ('a,'b) Graph.edge -> unit) =
        function channel -> function edge ->
    output_string channel ("\t" ^ (string_of_int
        (edge.Graph.src.Graph.node_contents)) ^ " -> " ^ (string_of_int
        (edge.Graph.dst.Graph.node_contents)) ^
        " [arrowhead=none,color=black];\n")
;;

let (dot_print : ('a,'b) Graph.t -> string -> unit) = function g -> function filename ->
    let channel = open_out filename in
    (* let channel = stdout in *)
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
    print_endline "Showing graph";
    let tmp = Filename.temp_file "graph" ".dot" in
    let tmp2 = Filename.temp_file "graph" ".ps" in
    (* dot_output g tmp; *)
    dot_print g tmp;
    ignore (Sys.command ("neato -Tps < " ^ tmp ^ " > " ^ tmp2 ^ " && gv " ^
    tmp2)) (*;
    Sys.remove tmp;
    Sys.remove tmp2 *)
;;

exception NeedArg;;
print_endline "Showing graph";
if Array.length Sys.argv < 2 then raise NeedArg;;
print_endline "Showing graph";
(* let g = Graph.read_graph_file Sys.argv.(1) in *)
let g = Graph.read_subgraph_file Sys.argv.(1) in
print_endline "Showing graph";
let mapfile = if Array.length Sys.argv > 2 then Some Sys.argv.(2) else None in
print_endline "Showing graph";
(match mapfile with
  None -> ()
| Some(x) -> fill_map x);
(* dot_print g "graph.dot" *)
print_endline "Showing graph 5";
show g

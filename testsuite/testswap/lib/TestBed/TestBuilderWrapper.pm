package TestBed::TestBuilderWrapper;
use Test::Builder;

sub set_test_builder_to_end_state {
  my ($test_count, %options) = @_;
  my $b = Test::Builder->new;
  $b->current_test($test_count); 
}

sub reset_test_builder {
  my ($test_count, %options) = @_;
  my $b = Test::Builder->new;
  $b->reset; 
  $b->use_numbers(0) if $options{no_numbers};
  if ($test_count) { $b->expected_tests($test_count); }
  else { $b->no_plan; }
}

sub setup_test_builder_ouputs {
  my ($out, $err) = @_;
  my $b = Test::Builder->new;
  $b->output($out);
  $b->fail_output($out);
  $b->todo_output($out);
}

1;

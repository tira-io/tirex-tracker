from tirex_tracker.utils import is_pyterrier_installed

if is_pyterrier_installed():
    from typing import Final, Iterable, Iterator, Mapping, Optional, Tuple, Union

    from pandas import DataFrame
    from pyterrier import Transformer
    from pyterrier.model import IterDict

    from . import ExportFormat, PathLike, tracking
    from ._utils.constants import ALL_MEASURES, Measure
    from ._utils.library import ResultEntry

    class TrackedTransformer(Transformer):
        transformer: Final[Transformer]
        measures: Final[Iterable[Measure]]
        poll_intervall_ms: Final[int]
        system_name: Final[Optional[str]]
        system_description: Final[Optional[str]]
        export_file_path: Final[Optional[PathLike]]
        export_format: Final[Optional[ExportFormat]]
        _results: Mapping[Measure, ResultEntry]

        def __init__(
            self,
            transformer: Transformer,
            measures: Iterable[Measure] = ALL_MEASURES,
            poll_interval_ms: int = 100,
            system_name: Optional[str] = None,
            system_description: Optional[str] = None,
            export_file_path: Optional[PathLike] = None,
            export_format: Optional[ExportFormat] = None,
        ) -> None:
            super().__init__()
            self.transformer = transformer
            self.measures = measures
            self.poll_intervall_ms = poll_interval_ms
            self.system_name = system_name
            self.system_description = system_description
            self.export_file_path = export_file_path
            self.export_format = export_format

        @property
        def results(self) -> Mapping[Measure, ResultEntry]:
            return self._results

        def transform(self, inp: DataFrame) -> DataFrame:
            with tracking(
                measures=self.measures,
                poll_intervall_ms=self.poll_intervall_ms,
                system_name=self.system_name,
                system_description=self.system_description,
                export_file_path=self.export_file_path,
                export_format=self.export_format,
            ) as tracking_results:
                results = self.transformer.transform(inp)
            self._results = tracking_results
            return results

        def transform_iter(self, inp: IterDict) -> IterDict:
            with tracking(
                measures=self.measures,
                poll_intervall_ms=self.poll_intervall_ms,
                system_name=self.system_name,
                system_description=self.system_description,
                export_file_path=self.export_file_path,
                export_format=self.export_format,
            ) as tracking_results:
                results = self.transformer.transform_iter(inp)
            self._results = tracking_results
            return results

        def transform_gen(
            self,
            input: DataFrame,
            batch_size=1,
            output_topics=False,
        ) -> Union[Iterator[DataFrame], Iterator[Tuple[DataFrame, DataFrame]]]:
            with tracking(
                measures=self.measures,
                poll_intervall_ms=self.poll_intervall_ms,
                system_name=self.system_name,
                system_description=self.system_description,
                export_file_path=self.export_file_path,
                export_format=self.export_format,
            ) as tracking_results:
                results = self.transformer.transform_gen(input, batch_size=batch_size, output_topics=output_topics)
            self._results = tracking_results
            return results
